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

#ifdef CONFIG_MTK_ROUND_CORNER_SUPPORT
#include "data_hw_roundedpattern.h"
#endif

#include "lcm_drv.h"
#include "disp_dts_gpio.h"

#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#endif

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

#define LCM_ID_NT36672C_TIANMA (0x03)

static struct LCM_UTIL_FUNCS lcm_util;

extern int BL_control_write_bytes(unsigned char addr, unsigned char value);
extern int Bias_power_write_bytes(unsigned char addr, unsigned char value);
extern void BDG_set_cmdq_V2_DSI0(void *cmdq, unsigned int cmd, unsigned char count,
			unsigned char *para_list, unsigned char force_update);

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define MDELAY(n)		(lcm_util.mdelay(n))
#define UDELAY(n)		(lcm_util.udelay(n))

#define dsi_set_cmdq_V4(para_tbl, size, force_update) \
	lcm_util.dsi_set_cmdq_V4(para_tbl, size, force_update)
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

#define LCM_DSI_CMD_MODE 0
#define FRAME_WIDTH 	(1080)
#define FRAME_HEIGHT 	(2460)

/* physical size in um */
#define LCM_PHYSICAL_WIDTH  (69174)
#define LCM_PHYSICAL_HEIGHT (157563)
#define LCM_DENSITY 		(420)

#define REGFLAG_DELAY			0xFFFC
#define REGFLAG_UDELAY			0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD
#define REGFLAG_RESET_LOW		0xFFFE
#define REGFLAG_RESET_HIGH		0xFFFF

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
	{REGFLAG_DELAY, 30, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 70, {} },
};

static struct LCM_setting_table init_setting[] = {
	{0XFF,0x01,{0X10}},
	{0XFB,0x01,{0X01}},
	{0X36,0x01,{0X00}},
	{0X3B,0x05,{0X03,0X14,0X36,0X04,0X04}},
	{0XB0,0x01,{0X00}},
	{0XC0,0x01,{0X03}},
	{0XC1,0x10,{0X89,0X28,0x00,0x14,0x00,0xAA,0x02,0x0E,0x00,0x71,0x00,0x07,0x05,0x0E,0x05,0x16}},
	{0XC2,0x02,{0X1B,0XA0}},
	{0XE9,0x01,{0X00}},
	{0XFF,0x01,{0X20}},
	{0XFB,0x01,{0X01}},
	{0X01,0x01,{0X66}},
	{0X06,0x01,{0X64}},
	{0X07,0x01,{0X28}},
	{0X17,0x01,{0X55}},
	{0X1B,0x01,{0X01}},
	{0X2D,0x01,{0X00}},
	{0X2F,0x01,{0X00}},
	{0X5C,0x01,{0X90}},
	{0X5E,0x01,{0XE6}},
	{0X69,0x01,{0XD0}},
	{0X6C,0x01,{0X66}},
	{0X6D,0x01,{0X66}},
	{0X89,0x01,{0X13}},
	{0X8A,0x01,{0X13}},
	{0X95,0x01,{0XD1}},
	{0X96,0x01,{0XD1}},
	{0XF2,0x01,{0X65}},
	{0XF4,0x01,{0X65}},
	{0XF6,0x01,{0X65}},
	{0XF8,0x01,{0X65}},
	{0XFF,0x01,{0X23}},
	{0XFB,0x01,{0X01}},
	{0X07,0x01,{0X00}},//26.6KHZ
	{0X08,0x01,{0X01}},//26.6KHZ
	{0X09,0x01,{0X01}},//26.6KHZ
	{0XFF,0x01,{0X24}},
	{0XFB,0x01,{0X01}},
	{0X00,0x01,{0X20}},
	{0X01,0x01,{0X20}},
	{0X02,0x01,{0X20}},
	{0X03,0x01,{0X13}},
	{0X04,0x01,{0X15}},
	{0X05,0x01,{0X17}},
	{0X06,0x01,{0X20}},
	{0X07,0x01,{0X20}},
	{0X08,0x01,{0X20}},
	{0X09,0x01,{0X26}},
	{0X0A,0x01,{0X27}},
	{0X0B,0x01,{0X28}},
	{0X0C,0x01,{0X24}},
	{0X0D,0x01,{0X01}},
	{0X0E,0x01,{0X2F}},
	{0X0F,0x01,{0X2D}},
	{0X10,0x01,{0X2E}},
	{0X11,0x01,{0X2C}},
	{0X12,0x01,{0X8B}},
	{0X13,0x01,{0X8C}},
	{0X14,0x01,{0X20}},
	{0X15,0x01,{0X20}},
	{0X16,0x01,{0X0F}},
	{0X17,0x01,{0X22}},
	{0X18,0x01,{0X20}},
	{0X19,0x01,{0X20}},
	{0X1A,0x01,{0X20}},
	{0X1B,0x01,{0X13}},
	{0X1C,0x01,{0X15}},
	{0X1D,0x01,{0X17}},
	{0X1E,0x01,{0X20}},
	{0X1F,0x01,{0X20}},
	{0X20,0x01,{0X20}},
	{0X21,0x01,{0X26}},
	{0X22,0x01,{0X27}},
	{0X23,0x01,{0X28}},
	{0X24,0x01,{0X24}},
	{0X25,0x01,{0X01}},
	{0X26,0x01,{0X2F}},
	{0X27,0x01,{0X2D}},
	{0X28,0x01,{0X2E}},
	{0X29,0x01,{0X2C}},
	{0X2A,0x01,{0X8B}},
	{0X2B,0x01,{0X8C}},
	{0X2D,0x01,{0X20}},
	{0X2F,0x01,{0X20}},
	{0X30,0x01,{0X0F}},
	{0X31,0x01,{0X22}},
	{0X32,0x01,{0X00}},
	{0X33,0x01,{0X03}},
	{0X35,0x01,{0X01}},
	{0X36,0x01,{0X01}},
	{0X36,0x01,{0X01}},
	{0X4D,0x01,{0X01}},
	{0X4E,0x01,{0X47}},
	{0X4F,0x01,{0X47}},
	{0X53,0x01,{0X47}},
	{0X71,0x01,{0X28}},
	{0X77,0x01,{0X80}},
	{0X79,0x01,{0X04}},
	{0X7A,0x01,{0X03}},
	{0X7B,0x01,{0X92}},
	{0X7D,0x01,{0X07}},
	{0X80,0x01,{0X07}},
	{0X81,0x01,{0X07}},
	{0X82,0x01,{0X13}},
	{0X83,0x01,{0X22}},
	{0X84,0x01,{0X31}},
	{0X85,0x01,{0X13}},
	{0X86,0x01,{0X22}},
	{0X87,0x01,{0X31}},
	{0X90,0x01,{0X13}},
	{0X91,0x01,{0X22}},
	{0X92,0x01,{0X31}},
	{0X93,0x01,{0X13}},
	{0X94,0x01,{0X22}},
	{0X95,0x01,{0X31}},
	{0X9C,0x01,{0XF4}},
	{0X9D,0x01,{0X01}},
	{0XA0,0x01,{0X12}},
	{0XA2,0x01,{0X12}},
	{0XA3,0x01,{0X03}},
	{0XA4,0x01,{0X07}},
	{0XA5,0x01,{0X07}},
	{0XC4,0x01,{0X40}},
	{0XC6,0x01,{0XC0}},
	{0XC9,0x01,{0X00}},
	{0XD9,0x01,{0X80}},
	{0XE9,0x01,{0X03}},
	{0XFF,0x01,{0X25}},
	{0XFB,0x01,{0X01}},
	{0X0F,0x01,{0X1B}},
	{0X18,0x01,{0X20}},
	{0X19,0x01,{0XE4}},
	{0X21,0x01,{0X40}},
	{0X66,0x01,{0X40}},
	{0X67,0x01,{0X29}},
	{0X68,0x01,{0X50}},
	{0X69,0x01,{0X60}},
	{0X6B,0x01,{0X00}},
	{0X71,0x01,{0X6D}},
	{0X77,0x01,{0X60}},
	{0X78,0x01,{0XA5}},
	{0X79,0x01,{0X7A}},
	{0X7D,0x01,{0X40}},
	{0X7E,0x01,{0X2D}},
	{0XC0,0x01,{0X4D}},
	{0XC1,0x01,{0XA9}},
	{0XC2,0x01,{0XD2}},
	{0XC4,0x01,{0X11}},
	{0XD6,0x01,{0X80}},
	{0XD7,0x01,{0X82}},
	{0XDD,0x01,{0X02}},
	{0XDA,0x01,{0X02}},
	{0XE0,0x01,{0X02}},
	{0XF0,0x01,{0X00}},
	{0XF1,0x01,{0X04}},
	{0XFF,0x01,{0X26}},
	{0XFB,0x01,{0X01}},
	{0X00,0x01,{0X10}},
	{0X01,0x01,{0XF8}},
	{0X03,0x01,{0X00}},
	{0X04,0x01,{0XF8}},
	{0X05,0x01,{0X08}},
	{0X06,0x01,{0X14}},
	{0X08,0x01,{0X14}},
	{0X14,0x01,{0X02}},
	{0X15,0x01,{0X01}},
	{0X74,0x01,{0XAF}},
	{0X81,0x01,{0X12}},
	{0X83,0x01,{0X03}},
	{0X84,0x01,{0X06}},
	{0X85,0x01,{0X01}},
	{0X86,0x01,{0X06}},
	{0X87,0x01,{0X01}},
	{0X88,0x01,{0X00}},
	{0X8A,0x01,{0X1A}},
	{0X8B,0x01,{0X11}},
	{0X8C,0x01,{0X24}},
	{0X8D,0x01,{0X33}},
	{0X8E,0x01,{0X42}},
	{0X8F,0x01,{0X11}},
	{0X90,0x01,{0X11}},
	{0X91,0x01,{0X11}},
	{0X9A,0x01,{0X80}},
	{0X9B,0x01,{0X04}},
	{0X9C,0x01,{0X00}},
	{0X9D,0x01,{0X00}},
	{0X9E,0x01,{0X00}},
	{0XFF,0x01,{0X27}},
	{0XFB,0x01,{0X01}},
	{0X01,0x01,{0X9C}},
	{0X20,0x01,{0X81}},
	{0X21,0x01,{0XDF}},
	{0X25,0x01,{0X82}},
	{0X26,0x01,{0X13}},
	{0X6E,0x01,{0X23}},
	{0X6F,0x01,{0X01}},
	{0X70,0x01,{0X00}},
	{0X71,0x01,{0X00}},
	{0X72,0x01,{0X00}},
	{0X73,0x01,{0X21}},
	{0X74,0x01,{0X03}},
	{0X75,0x01,{0X00}},
	{0X76,0x01,{0X00}},
	{0X77,0x01,{0X00}},
	{0X7D,0x01,{0X09}},
	{0X7E,0x01,{0X9F}},
	{0X80,0x01,{0X23}},
	{0X82,0x01,{0X09}},
	{0X83,0x01,{0X9F}},
	{0X88,0x01,{0X03}},
	{0X89,0x01,{0X01}},
	{0XE3,0x01,{0X02}},
	{0XE4,0x01,{0XCE}},
	{0XE9,0x01,{0X03}},
	{0XEA,0x01,{0X1C}},
	{0XFF,0x01,{0X2A}},
	{0XFB,0x01,{0X01}},
	{0X00,0x01,{0X91}},
	{0X03,0x01,{0X20}},
	{0X04,0x01,{0X4C}},
	{0X07,0x01,{0X40}},
	{0X0A,0x01,{0X70}},
	{0X0C,0x01,{0X09}},
	{0X0D,0x01,{0X40}},
	{0X0E,0x01,{0X02}},
	{0X0F,0x01,{0X00}},
	{0X11,0x01,{0XF6}},
	{0X15,0x01,{0X0F}},
	{0X16,0x01,{0X86}},
	{0X19,0x01,{0X0F}},
	{0X1A,0x01,{0X5A}},
	{0X1B,0x01,{0X14}},
	{0X1D,0x01,{0X36}},
	{0X1E,0x01,{0X4D}},
	{0X1F,0x01,{0X4D}},
	{0X20,0x01,{0X4D}},
	{0X28,0x01,{0XF8}},
	{0X29,0x01,{0X06}},
	{0X2A,0x01,{0X2A}},
	{0X2B,0x01,{0X00}},
	{0X2D,0x01,{0X06}},
	{0X2F,0x01,{0X01}},
	{0X30,0x01,{0X45}},
	{0X31,0x01,{0XE6}},
	{0X33,0x01,{0X09}},
	{0X34,0x01,{0XFA}},
	{0X35,0x01,{0X33}},
	{0X36,0x01,{0X0C}},
	{0X36,0x01,{0X0C}},
	{0X37,0x01,{0XF5}},
	{0X38,0x01,{0X37}},
	{0X39,0x01,{0X08}},
	{0X3A,0x01,{0X45}},
	{0X45,0x01,{0X09}},
	{0X46,0x01,{0X40}},
	{0X47,0x01,{0X02}},
	{0X48,0x01,{0X00}},
	{0X4A,0x01,{0XF6}},
	{0X4E,0x01,{0X0F}},
	{0X4F,0x01,{0X86}},
	{0X52,0x01,{0X0F}},
	{0X53,0x01,{0X5A}},
	{0X54,0x01,{0X14}},
	{0X56,0x01,{0X36}},
	{0X57,0x01,{0X7C}},
	{0X58,0x01,{0X7C}},
	{0X59,0x01,{0X7C}},
	{0X60,0x01,{0X00}},
	{0X61,0x01,{0X04}},
	{0X62,0x01,{0X68}},
	{0X63,0x01,{0X00}},
	{0X65,0x01,{0X01}},
	{0X66,0x01,{0X00}},
	{0X67,0x01,{0X01}},
	{0X68,0x01,{0X01}},
	{0X6A,0x01,{0X01}},
	{0X6B,0x01,{0X71}},
	{0X6C,0x01,{0X71}},
	{0X6D,0x01,{0X71}},
	{0X6E,0x01,{0X00}},
	{0X6F,0x01,{0X0A}},
	{0X70,0x01,{0X80}},
	{0X71,0x01,{0X80}},
	{0X83,0x01,{0X32}},
	{0X84,0x01,{0X01}},
	{0X87,0x01,{0X29}},
	{0X88,0x01,{0XFD}},
	{0X8B,0x01,{0X00}},
	{0XFF,0x01,{0X2C}},
	{0XFB,0x01,{0X01}},
	{0X00,0x01,{0X03}},
	{0X01,0x01,{0X03}},
	{0X02,0x01,{0X03}},
	{0X03,0x01,{0X1E}},
	{0X04,0x01,{0X1E}},
	{0X05,0x01,{0X1E}},
	{0X0D,0x01,{0X01}},
	{0X0E,0x01,{0X01}},
	{0X17,0x01,{0X6F}},
	{0X18,0x01,{0X6F}},
	{0X19,0x01,{0X6F}},
	{0X2D,0x01,{0XAF}},
	{0X2F,0x01,{0X10}},
	{0X30,0x01,{0XF8}},
	{0X32,0x01,{0X00}},
	{0X33,0x01,{0XF8}},
	{0X35,0x01,{0X20}},
	{0X37,0x01,{0X20}},
	{0X37,0x01,{0X20}},
	{0X4D,0x01,{0X1E}},
	{0X4E,0x01,{0X03}},
	{0X4F,0x01,{0X0B}},
	{0XFF,0x01,{0XE0}},
	{0XFB,0x01,{0X01}},
	{0X35,0x01,{0X82}},
	{0X85,0x01,{0X32}},
	{0X85,0x01,{0X32}},
	{0XFF,0x01,{0XF0}},
	{0XFB,0x01,{0X01}},
	{0X1C,0x01,{0X01}},
	{0X33,0x01,{0X01}},
	{0X5A,0x01,{0X00}},
	{0X9F,0x01,{0X19}},
	{0XFF,0x01,{0XD0}},
	{0XFB,0x01,{0X01}},
	{0X53,0x01,{0X22}},
	{0X54,0x01,{0X02}},
	{0XFF,0x01,{0XC0}},
	{0XFB,0x01,{0X01}},
	{0X9C,0x01,{0X11}},
	{0X9D,0x01,{0X11}},
	{0XFF,0x01,{0X2B}},
	{0XFB,0x01,{0X01}},
	{0XB7,0x01,{0X17}},
	{0XB8,0x01,{0X05}},
	{0XC0,0x01,{0X02}},
	{0XFF,0x01,{0X10}},
	{0XFB,0x01,{0X01}},
	{0X51,0x01,{0x00}},
	{0X53,0x01,{0X2C}},
	{0X55,0x01,{0X00}},

	{0X11,0x00,{}},
	{REGFLAG_DELAY,100,{}},
	{0X29,0x00,{}},
	{REGFLAG_DELAY,40,{}},
/*
	{0xFF,0x01,{0x10}},
	{0xFB,0x01,{0x01}},
	{0x28,0x01,{}},
	{0x10,0x01,{}},
	{0xFF,0x01,{0x27}},
	{0xFB,0x01,{0x01}},
	{0xF9,0x01,{0x00}},
	{0xFF,0x01,{0x20}},
	{0xFB,0x01,{0x01}},
	{0x74,0x01,{0x00}},
	{0x75,0x01,{0x00}},
	{0x76,0x01,{0x00}},
	{0x77,0x01,{0x01}},
	{0x78,0x01,{0x00}},
	{0x79,0x01,{0x00}},
	{0x7A,0x01,{0x00}},
	{0x7B,0x01,{0x00}},
	{0x86,0x01,{0x03}},
	{REGFLAG_DELAY,100,{}},
	{0xFF,0x01,{0x10}},
	{0xFB,0x01,{0x01}},
*/
};
/*
static struct LCM_setting_table bl_level[] = {
	{0x51, 1, {0xFF} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};
*/
static struct LCM_setting_table_V4 BL_Level[] = {
	{0x15, 0x51, 1, {0xFF}, 0 },
};
static void push_table(void *cmdq, struct LCM_setting_table *table,
		       unsigned int count, unsigned char force_update,int IsMT6382)
{
	unsigned int i;
	unsigned int cmd;

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
			if(IsMT6382)
				BDG_set_cmdq_V2_DSI0(cmdq, cmd, table[i].count,table[i].para_list, force_update);
			else
				dsi_set_cmdq_V22(cmdq, cmd, table[i].count,table[i].para_list, force_update);
		}
	}
}


static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}


#ifdef CONFIG_MTK_HIGH_FRAME_RATE
static void lcm_dfps_int(struct LCM_DSI_PARAMS *dsi)
{
	struct dfps_info *dfps_params = dsi->dfps_params;

	dsi->dfps_enable = 1;
	dsi->dfps_default_fps = 6000;/*real fps * 100, to support float*/
	dsi->dfps_def_vact_tim_fps = 9000;/*real vact timing fps * 100*/

	/* DPFS_LEVEL0 */
	dfps_params[0].level = DFPS_LEVEL0;
	dfps_params[0].fps = 6000;/*real fps * 100, to support float*/
	dfps_params[0].vact_timing_fps = 9000;/*real vact timing fps * 100*/
	dfps_params[0].vertical_frontporch = 1321;
	dfps_params[0].vertical_frontporch_for_low_power = 0;

	/* DPFS_LEVEL1 */
	dfps_params[1].level = DFPS_LEVEL1;
	dfps_params[1].fps = 9000;/*real fps * 100, to support float*/
	dfps_params[1].vact_timing_fps = 9000;/*real vact timing fps * 100*/
	dfps_params[1].vertical_frontporch = 54;
	dfps_params[1].vertical_frontporch_for_low_power = 0;

	dsi->dfps_num = 2;
}
#endif


static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->physical_height = LCM_PHYSICAL_HEIGHT/1000;
	params->physical_width = LCM_PHYSICAL_WIDTH/1000;

	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
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
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	/* video mode timing */
	params->dsi.vertical_sync_active = 10;
	params->dsi.vertical_backporch = 10;
	params->dsi.vertical_frontporch = 1321;
	//params->dsi.vertical_frontporch_for_low_power = 2500;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 22;
	params->dsi.horizontal_backporch = 30;
	params->dsi.horizontal_frontporch = 143;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_disable = 1;
	params->dsi.dsc_enable = 0;

	params->dsi.bdg_ssc_disable = 1;
	params->dsi.bdg_dsc_enable = 1;
	params->dsi.dsc_params.ver = 11;
	params->dsi.dsc_params.slice_mode = 1;
	params->dsi.dsc_params.rgb_swap = 0;
	params->dsi.dsc_params.dsc_cfg = 34;
	params->dsi.dsc_params.rct_on = 1;
	params->dsi.dsc_params.bit_per_channel = 8;
	params->dsi.dsc_params.dsc_line_buf_depth = 9;
	params->dsi.dsc_params.bp_enable = 1;
	params->dsi.dsc_params.bit_per_pixel = 128;
	params->dsi.dsc_params.pic_height = 2460;
	params->dsi.dsc_params.pic_width = 1080;
	params->dsi.dsc_params.slice_height = 20;
	params->dsi.dsc_params.slice_width = 540;
	params->dsi.dsc_params.chunk_size = 540;
	params->dsi.dsc_params.xmit_delay = 170;
	params->dsi.dsc_params.dec_delay = 526;
	params->dsi.dsc_params.scale_value = 32;
	params->dsi.dsc_params.increment_interval = 113;
	params->dsi.dsc_params.decrement_interval = 7;
	params->dsi.dsc_params.line_bpg_offset = 12;
	params->dsi.dsc_params.nfl_bpg_offset = 1294;
	params->dsi.dsc_params.slice_bpg_offset = 1302;
	params->dsi.dsc_params.initial_offset = 6144;
	params->dsi.dsc_params.final_offset = 7072;
	params->dsi.dsc_params.flatness_minqp = 3;
	params->dsi.dsc_params.flatness_maxqp = 12;
	params->dsi.dsc_params.rc_model_size = 8192;
	params->dsi.dsc_params.rc_edge_factor = 6;
	params->dsi.dsc_params.rc_quant_incr_limit0 = 11;
	params->dsi.dsc_params.rc_quant_incr_limit1 = 11;
	params->dsi.dsc_params.rc_tgt_offset_hi = 3;
	params->dsi.dsc_params.rc_tgt_offset_lo = 3;

	/* this value must be in MTK suggested table */
	params->dsi.PLL_CLOCK = 400;
	params->dsi.data_rate = 800;

	/*params->dsi.clk_lp_per_line_enable = 0;*/

	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;

	params->dsi.lane_swap_en = 0;

#ifdef CONFIG_MTK_ROUND_CORNER_SUPPORT
	params->round_corner_en = 1;
	params->corner_pattern_height = ROUND_CORNER_H_TOP;
	params->corner_pattern_height_bot = ROUND_CORNER_H_BOT;
	params->corner_pattern_tp_size = sizeof(top_rc_pattern);
	params->corner_pattern_lt_addr = (void *)top_rc_pattern;
#endif

#ifdef CONFIG_MTK_HIGH_FRAME_RATE
	/****DynFPS start****/
	lcm_dfps_int(&(params->dsi));
	/****DynFPS end****/
#endif

}


static void lcm_init(void)
{
	pr_info("%s enter\n", __func__);

	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT0);
	MDELAY(10);

	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BIAS_ENP_OUT1);
	Bias_power_write_bytes(0x00,0x0F);
	MDELAY(5);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BIAS_ENN_OUT1);
	Bias_power_write_bytes(0x01,0x0F);
	MDELAY(5);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
	MDELAY(15);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT0);
	MDELAY(15);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
	MDELAY(50);

	push_table(NULL, init_setting,
		sizeof(init_setting)/sizeof(struct LCM_setting_table), 1, 1);

	MDELAY(40);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BL_EN_OUT1);
	BL_control_write_bytes(0x02,0x01);
	BL_control_write_bytes(0x10,0x07);
	BL_control_write_bytes(0x19,0x07);
	BL_control_write_bytes(0x1A,0x04);
	BL_control_write_bytes(0x1C,0x0E);
	BL_control_write_bytes(0x24,0x02);
	BL_control_write_bytes(0x22,0x0F);
	BL_control_write_bytes(0x23,0xFF);

	pr_info("%s end\n", __func__);

}

static void lcm_suspend(void)
{
	pr_info("%s enter\n", __func__);

	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BL_EN_OUT0);

	push_table(NULL, lcm_suspend_setting,
		sizeof(lcm_suspend_setting)/sizeof(struct LCM_setting_table),
		1, 1);

	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BIAS_ENN_OUT0);
	MDELAY(1);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BIAS_ENP_OUT0);

	pr_info("%s end\n", __func__);

}

static void lcm_resume(void)
{
	pr_info("%s enter\n", __func__);

	lcm_init();

	pr_info("%s end\n", __func__);
}

static unsigned int lcm_ata_check(unsigned char *buffer)
{
	return 1;
}

static void lcm_setbacklight_cmdq(void *handle,unsigned int level)
{

	pr_info("%s,nt36672c tianma backlight: level = %d\n", __func__, level);
	if(level < 30 && level != 0)
		level = 30;

	BL_Level[0].para_list[0] = level;

	dsi_set_cmdq_V4(BL_Level,
				sizeof(BL_Level)
				/ sizeof(struct LCM_setting_table_V4), 1);

}

static void *lcm_switch_mode(int mode)
{
	return NULL;
}


struct LCM_DRIVER nt36672c_fhdp_dsi_vdo_tianma_60_90HZ_lcm_drv = {
	.name = "nt36672c_fhdp_dsi_vdo_tianma_60_90HZ_lcm_drv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.set_backlight_cmdq= lcm_setbacklight_cmdq,
	.ata_check = lcm_ata_check,
	.switch_mode = lcm_switch_mode,

};
