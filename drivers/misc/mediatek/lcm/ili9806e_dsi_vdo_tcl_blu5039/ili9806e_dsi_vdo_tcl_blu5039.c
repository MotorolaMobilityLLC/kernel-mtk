/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif
#include "lcm_drv.h"
#include <mt_gpio.h>
#include "mach/gpio_const.h"
#include "upmu_common.h"
#include <mach/upmu_sw.h>

/* --------------------------------------------------------------------------- */
/* Local Constants */
/* --------------------------------------------------------------------------- */

#define FRAME_WIDTH  (480)
#define FRAME_HEIGHT (854)

#define ILI9806E 0x9806

#define ONTIM_MODE   1
static u8 exit_sleep[] = { 0x11 };
static u8 display_on[] = { 0x29 };

/* #define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0])) */
/* --------------------------------------------------------------------------- */
/* Local Variables */
/* --------------------------------------------------------------------------- */

static LCM_UTIL_FUNCS lcm_util = { 0 };

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

#define DSI_CMD_DCS							(0xF0)
#define DSI_CMD_GEN							(0xF1)
#define DSI_CMD_SET_PKT						(0xF2)
#define DSI_CMD_READ							(0xF3)
#define DSI_CMD_READ_BUFF					(0xF4)
#define DSI_CMD_AUTO							(0xF5)

#define REGFLAG_DELAY								0XFD
#define REGFLAG_END_OF_TABLE							0xFE	/* END OF REGISTERS MARKER */

#ifndef GPIO_LCM_RST
#define GPIO_LCD_RST_PIN         (GPIO70 | 0x80000000)
#else
#define GPIO_LCD_RST_PIN         GPIO_LCM_RST
#endif
/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq_V2_DCS(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2_DCS(cmd, count, ppara, force_update)
#define dsi_set_cmdq_V2_generic(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2_generic(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
	lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) \
	lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
	lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg_v2(cmd, buffer, buffer_size) \
	lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)



/* #define LCM_DSI_CMD_MODE */

struct LCM_setting_table {
#if ONTIM_MODE
	u8 data_type;
	unsigned int delay;	/* time to delay */
	unsigned int length;	/* cmds length */
	u8 *data;
	/* unsigned cmd; */
	/* unsigned char count; */
	/* unsigned char para_list[64]; */
#else
	unsigned cmd;
	unsigned char count;
	unsigned char para_list[64];
#endif
};

#if ONTIM_MODE
static u8 lcd_tcl_ili9806e_cmd_1[] = { 0xFF, 0xFF, 0x98, 0x06, 0x04, 0x01 };
static u8 lcd_tcl_ili9806e_cmd_2[] = { 0x08, 0X10 };
static u8 lcd_tcl_ili9806e_cmd_3[] = { 0x21, 0X01 };
static u8 lcd_tcl_ili9806e_cmd_4[] = { 0x30, 0X01 };
static u8 lcd_tcl_ili9806e_cmd_5[] = { 0x31, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_6[] = { 0x40, 0X11 };
static u8 lcd_tcl_ili9806e_cmd_7[] = { 0x41, 0X44 };
static u8 lcd_tcl_ili9806e_cmd_8[] = { 0x42, 0X03 };
static u8 lcd_tcl_ili9806e_cmd_9[] = { 0x43, 0X09 };
static u8 lcd_tcl_ili9806e_cmd_10[] = { 0x44, 0X06 };
static u8 lcd_tcl_ili9806e_cmd_11[] = { 0x50, 0X80 };
static u8 lcd_tcl_ili9806e_cmd_12[] = { 0x51, 0X80 };
static u8 lcd_tcl_ili9806e_cmd_13[] = { 0x52, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_14[] = { 0x53, 0X30 };
static u8 lcd_tcl_ili9806e_cmd_15[] = { 0x60, 0X07 };
static u8 lcd_tcl_ili9806e_cmd_16[] = { 0x61, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_17[] = { 0x62, 0X07 };
static u8 lcd_tcl_ili9806e_cmd_18[] = { 0x63, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_19[] = { 0xA0, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_20[] = { 0xA1, 0X09 };
static u8 lcd_tcl_ili9806e_cmd_21[] = { 0xA2, 0X17 };
static u8 lcd_tcl_ili9806e_cmd_22[] = { 0xA3, 0X13 };
static u8 lcd_tcl_ili9806e_cmd_23[] = { 0xA4, 0X0C };
static u8 lcd_tcl_ili9806e_cmd_24[] = { 0xA5, 0X1B };
static u8 lcd_tcl_ili9806e_cmd_25[] = { 0xA6, 0X09 };
static u8 lcd_tcl_ili9806e_cmd_26[] = { 0xA7, 0X07 };
static u8 lcd_tcl_ili9806e_cmd_27[] = { 0xA8, 0X02 };
static u8 lcd_tcl_ili9806e_cmd_28[] = { 0xA9, 0X05 };
static u8 lcd_tcl_ili9806e_cmd_29[] = { 0xAA, 0X06 };
static u8 lcd_tcl_ili9806e_cmd_30[] = { 0xAB, 0X04 };
static u8 lcd_tcl_ili9806e_cmd_31[] = { 0xAC, 0X0E };
static u8 lcd_tcl_ili9806e_cmd_32[] = { 0xAD, 0X2d };
static u8 lcd_tcl_ili9806e_cmd_33[] = { 0xAE, 0X26 };
static u8 lcd_tcl_ili9806e_cmd_34[] = { 0xAF, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_35[] = { 0xC0, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_36[] = { 0xC1, 0X07 };
static u8 lcd_tcl_ili9806e_cmd_37[] = { 0xC2, 0X12 };
static u8 lcd_tcl_ili9806e_cmd_38[] = { 0xC3, 0X10 };
static u8 lcd_tcl_ili9806e_cmd_39[] = { 0xC4, 0X09 };
static u8 lcd_tcl_ili9806e_cmd_40[] = { 0xC5, 0X17 };
static u8 lcd_tcl_ili9806e_cmd_41[] = { 0xC6, 0X09 };
static u8 lcd_tcl_ili9806e_cmd_42[] = { 0xC7, 0X08 };
static u8 lcd_tcl_ili9806e_cmd_43[] = { 0xC8, 0X03 };
static u8 lcd_tcl_ili9806e_cmd_44[] = { 0xC9, 0X09 };
static u8 lcd_tcl_ili9806e_cmd_45[] = { 0xCA, 0X05 };
static u8 lcd_tcl_ili9806e_cmd_46[] = { 0xCB, 0X02 };
static u8 lcd_tcl_ili9806e_cmd_47[] = { 0xCC, 0X09 };
static u8 lcd_tcl_ili9806e_cmd_48[] = { 0xCD, 0X27 };
static u8 lcd_tcl_ili9806e_cmd_49[] = { 0xCE, 0X24 };
static u8 lcd_tcl_ili9806e_cmd_50[] = { 0xCF, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_51[] = { 0xFF, 0xFF, 0x98, 0x06, 0x04, 0x06 };
static u8 lcd_tcl_ili9806e_cmd_52[] = { 0x00, 0X21 };
static u8 lcd_tcl_ili9806e_cmd_53[] = { 0x01, 0X04 };
static u8 lcd_tcl_ili9806e_cmd_54[] = { 0x02, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_55[] = { 0x03, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_56[] = { 0x04, 0X16 };
static u8 lcd_tcl_ili9806e_cmd_57[] = { 0x05, 0X16 };
static u8 lcd_tcl_ili9806e_cmd_58[] = { 0x06, 0X80 };
static u8 lcd_tcl_ili9806e_cmd_59[] = { 0x07, 0X02 };
static u8 lcd_tcl_ili9806e_cmd_60[] = { 0x08, 0X07 };
static u8 lcd_tcl_ili9806e_cmd_61[] = { 0x09, 0X90 };
static u8 lcd_tcl_ili9806e_cmd_62[] = { 0x0A, 0X01 };
static u8 lcd_tcl_ili9806e_cmd_63[] = { 0x0B, 0X06 };
static u8 lcd_tcl_ili9806e_cmd_64[] = { 0x0C, 0X15 };
static u8 lcd_tcl_ili9806e_cmd_65[] = { 0x0D, 0X15 };
static u8 lcd_tcl_ili9806e_cmd_66[] = { 0x0E, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_67[] = { 0x0F, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_68[] = { 0x10, 0X77 };
static u8 lcd_tcl_ili9806e_cmd_69[] = { 0x11, 0XF0 };
static u8 lcd_tcl_ili9806e_cmd_70[] = { 0x12, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_71[] = { 0x13, 0X87 };
static u8 lcd_tcl_ili9806e_cmd_72[] = { 0x14, 0X87 };
static u8 lcd_tcl_ili9806e_cmd_73[] = { 0x15, 0XC0 };
static u8 lcd_tcl_ili9806e_cmd_74[] = { 0x16, 0X08 };
static u8 lcd_tcl_ili9806e_cmd_75[] = { 0x17, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_76[] = { 0x18, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_77[] = { 0x19, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_78[] = { 0x1A, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_79[] = { 0x1B, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_80[] = { 0x1C, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_81[] = { 0x1D, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_82[] = { 0x20, 0X01 };
static u8 lcd_tcl_ili9806e_cmd_83[] = { 0x21, 0X23 };
static u8 lcd_tcl_ili9806e_cmd_84[] = { 0x22, 0X45 };
static u8 lcd_tcl_ili9806e_cmd_85[] = { 0x23, 0X67 };
static u8 lcd_tcl_ili9806e_cmd_86[] = { 0x24, 0X01 };
static u8 lcd_tcl_ili9806e_cmd_87[] = { 0x25, 0X23 };
static u8 lcd_tcl_ili9806e_cmd_88[] = { 0x26, 0X45 };
static u8 lcd_tcl_ili9806e_cmd_89[] = { 0x27, 0X67 };
static u8 lcd_tcl_ili9806e_cmd_90[] = { 0x30, 0X11 };
static u8 lcd_tcl_ili9806e_cmd_91[] = { 0x31, 0X22 };
static u8 lcd_tcl_ili9806e_cmd_92[] = { 0x32, 0X11 };
static u8 lcd_tcl_ili9806e_cmd_93[] = { 0x33, 0X00 };
static u8 lcd_tcl_ili9806e_cmd_94[] = { 0x34, 0X66 };
static u8 lcd_tcl_ili9806e_cmd_95[] = { 0x35, 0X88 };
static u8 lcd_tcl_ili9806e_cmd_96[] = { 0x36, 0X22 };
static u8 lcd_tcl_ili9806e_cmd_97[] = { 0x37, 0X22 };
static u8 lcd_tcl_ili9806e_cmd_98[] = { 0x38, 0XAA };
static u8 lcd_tcl_ili9806e_cmd_99[] = { 0x39, 0XCC };
static u8 lcd_tcl_ili9806e_cmd_100[] = { 0x3A, 0XBB };
static u8 lcd_tcl_ili9806e_cmd_101[] = { 0x3B, 0XDD };
static u8 lcd_tcl_ili9806e_cmd_102[] = { 0x3C, 0X22 };
static u8 lcd_tcl_ili9806e_cmd_103[] = { 0x3D, 0X22 };
static u8 lcd_tcl_ili9806e_cmd_104[] = { 0x3E, 0X22 };
static u8 lcd_tcl_ili9806e_cmd_105[] = { 0x3F, 0X22 };
static u8 lcd_tcl_ili9806e_cmd_106[] = { 0x40, 0X22 };
static u8 lcd_tcl_ili9806e_cmd_107[] = { 0x52, 0X10 };
static u8 lcd_tcl_ili9806e_cmd_108[] = { 0x53, 0X10 };
static u8 lcd_tcl_ili9806e_cmd_109[] = { 0xFF, 0xFF, 0x98, 0x06, 0x04, 0x05 };
static u8 lcd_tcl_ili9806e_cmd_110[] = { 0x00, 0x03 };

static u8 lcd_tcl_ili9806e_cmd_111[] = { 0xFF, 0xFF, 0x98, 0x06, 0x04, 0x07 };
static u8 lcd_tcl_ili9806e_cmd_112[] = { 0x17, 0X22 };
static u8 lcd_tcl_ili9806e_cmd_113[] = { 0x02, 0X77 };
static u8 lcd_tcl_ili9806e_cmd_114[] = { 0xE1, 0X79 };
static u8 lcd_tcl_ili9806e_cmd_115[] = { 0xFF, 0xFF, 0x98, 0x06, 0x04, 0x00 };

static u8 lcd_tcl_ili9806e_cmd_116[] = { 0x35, 0x00 };
static u8 lcd_tcl_ili9806e_cmd_117[] = { 0x51, 0x00 };
static u8 lcd_tcl_ili9806e_cmd_118[] = { 0x53, 0x24 };
static u8 lcd_tcl_ili9806e_cmd_119[] = { 0x55, 0x00 };

static u8 lcd_backlight_control_cmd[] = { 0x51, 0x00 };
#endif


static struct LCM_setting_table lcm_initialization_setting[] = {
#if ONTIM_MODE
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_1), lcd_tcl_ili9806e_cmd_1},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_2), lcd_tcl_ili9806e_cmd_2},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_3), lcd_tcl_ili9806e_cmd_3},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_4), lcd_tcl_ili9806e_cmd_4},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_5), lcd_tcl_ili9806e_cmd_5},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_6), lcd_tcl_ili9806e_cmd_6},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_7), lcd_tcl_ili9806e_cmd_7},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_8), lcd_tcl_ili9806e_cmd_8},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_9), lcd_tcl_ili9806e_cmd_9},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_10), lcd_tcl_ili9806e_cmd_10},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_11), lcd_tcl_ili9806e_cmd_11},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_12), lcd_tcl_ili9806e_cmd_12},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_13), lcd_tcl_ili9806e_cmd_13},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_14), lcd_tcl_ili9806e_cmd_14},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_15), lcd_tcl_ili9806e_cmd_15},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_16), lcd_tcl_ili9806e_cmd_16},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_17), lcd_tcl_ili9806e_cmd_17},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_18), lcd_tcl_ili9806e_cmd_18},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_19), lcd_tcl_ili9806e_cmd_19},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_20), lcd_tcl_ili9806e_cmd_20},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_21), lcd_tcl_ili9806e_cmd_21},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_22), lcd_tcl_ili9806e_cmd_22},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_23), lcd_tcl_ili9806e_cmd_23},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_24), lcd_tcl_ili9806e_cmd_24},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_25), lcd_tcl_ili9806e_cmd_25},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_26), lcd_tcl_ili9806e_cmd_26},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_27), lcd_tcl_ili9806e_cmd_27},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_28), lcd_tcl_ili9806e_cmd_28},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_29), lcd_tcl_ili9806e_cmd_29},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_30), lcd_tcl_ili9806e_cmd_30},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_31), lcd_tcl_ili9806e_cmd_31},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_32), lcd_tcl_ili9806e_cmd_32},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_33), lcd_tcl_ili9806e_cmd_33},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_34), lcd_tcl_ili9806e_cmd_34},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_35), lcd_tcl_ili9806e_cmd_35},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_36), lcd_tcl_ili9806e_cmd_36},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_37), lcd_tcl_ili9806e_cmd_37},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_38), lcd_tcl_ili9806e_cmd_38},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_39), lcd_tcl_ili9806e_cmd_39},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_40), lcd_tcl_ili9806e_cmd_40},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_41), lcd_tcl_ili9806e_cmd_41},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_42), lcd_tcl_ili9806e_cmd_42},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_43), lcd_tcl_ili9806e_cmd_43},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_44), lcd_tcl_ili9806e_cmd_44},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_45), lcd_tcl_ili9806e_cmd_45},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_46), lcd_tcl_ili9806e_cmd_46},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_47), lcd_tcl_ili9806e_cmd_47},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_48), lcd_tcl_ili9806e_cmd_48},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_49), lcd_tcl_ili9806e_cmd_49},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_50), lcd_tcl_ili9806e_cmd_50},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_51), lcd_tcl_ili9806e_cmd_51},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_52), lcd_tcl_ili9806e_cmd_52},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_53), lcd_tcl_ili9806e_cmd_53},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_54), lcd_tcl_ili9806e_cmd_54},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_55), lcd_tcl_ili9806e_cmd_55},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_56), lcd_tcl_ili9806e_cmd_56},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_57), lcd_tcl_ili9806e_cmd_57},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_58), lcd_tcl_ili9806e_cmd_58},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_59), lcd_tcl_ili9806e_cmd_59},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_60), lcd_tcl_ili9806e_cmd_60},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_61), lcd_tcl_ili9806e_cmd_61},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_62), lcd_tcl_ili9806e_cmd_62},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_63), lcd_tcl_ili9806e_cmd_63},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_64), lcd_tcl_ili9806e_cmd_64},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_65), lcd_tcl_ili9806e_cmd_65},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_66), lcd_tcl_ili9806e_cmd_66},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_67), lcd_tcl_ili9806e_cmd_67},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_68), lcd_tcl_ili9806e_cmd_68},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_69), lcd_tcl_ili9806e_cmd_69},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_70), lcd_tcl_ili9806e_cmd_70},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_71), lcd_tcl_ili9806e_cmd_71},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_72), lcd_tcl_ili9806e_cmd_72},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_73), lcd_tcl_ili9806e_cmd_73},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_74), lcd_tcl_ili9806e_cmd_74},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_75), lcd_tcl_ili9806e_cmd_75},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_76), lcd_tcl_ili9806e_cmd_76},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_77), lcd_tcl_ili9806e_cmd_77},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_78), lcd_tcl_ili9806e_cmd_78},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_79), lcd_tcl_ili9806e_cmd_79},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_80), lcd_tcl_ili9806e_cmd_80},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_81), lcd_tcl_ili9806e_cmd_81},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_82), lcd_tcl_ili9806e_cmd_82},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_83), lcd_tcl_ili9806e_cmd_83},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_84), lcd_tcl_ili9806e_cmd_84},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_85), lcd_tcl_ili9806e_cmd_85},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_86), lcd_tcl_ili9806e_cmd_86},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_87), lcd_tcl_ili9806e_cmd_87},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_88), lcd_tcl_ili9806e_cmd_88},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_89), lcd_tcl_ili9806e_cmd_89},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_90), lcd_tcl_ili9806e_cmd_90},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_91), lcd_tcl_ili9806e_cmd_91},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_92), lcd_tcl_ili9806e_cmd_92},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_93), lcd_tcl_ili9806e_cmd_93},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_94), lcd_tcl_ili9806e_cmd_94},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_95), lcd_tcl_ili9806e_cmd_95},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_96), lcd_tcl_ili9806e_cmd_96},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_97), lcd_tcl_ili9806e_cmd_97},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_98), lcd_tcl_ili9806e_cmd_98},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_99), lcd_tcl_ili9806e_cmd_99},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_100), lcd_tcl_ili9806e_cmd_100},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_101), lcd_tcl_ili9806e_cmd_101},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_102), lcd_tcl_ili9806e_cmd_102},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_103), lcd_tcl_ili9806e_cmd_103},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_104), lcd_tcl_ili9806e_cmd_104},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_105), lcd_tcl_ili9806e_cmd_105},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_106), lcd_tcl_ili9806e_cmd_106},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_107), lcd_tcl_ili9806e_cmd_107},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_108), lcd_tcl_ili9806e_cmd_108},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_109), lcd_tcl_ili9806e_cmd_109},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_110), lcd_tcl_ili9806e_cmd_110},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_111), lcd_tcl_ili9806e_cmd_111},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_112), lcd_tcl_ili9806e_cmd_112},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_113), lcd_tcl_ili9806e_cmd_113},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_114), lcd_tcl_ili9806e_cmd_114},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_115), lcd_tcl_ili9806e_cmd_115},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_116), lcd_tcl_ili9806e_cmd_116},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_117), lcd_tcl_ili9806e_cmd_117},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_118), lcd_tcl_ili9806e_cmd_118},
	{DSI_CMD_AUTO, 0, sizeof(lcd_tcl_ili9806e_cmd_119), lcd_tcl_ili9806e_cmd_119},
	{DSI_CMD_AUTO, 0, sizeof(exit_sleep), exit_sleep},
	{REGFLAG_DELAY, 120, 0, NULL},
	{DSI_CMD_AUTO, 0, sizeof(display_on), display_on},
	{REGFLAG_DELAY, 20, 0, NULL},

#else
	/*
	   Note :

	   Data ID will depends on the following rule.

	   count of parameters > 1      => Data ID = 0x39
	   count of parameters = 1      => Data ID = 0x15
	   count of parameters = 0      => Data ID = 0x05

	   Structure Format :

	   {DCS command, count of parameters, {parameter list}}
	   {REGFLAG_DELAY, milliseconds of time, {}},

	   ...

	   Setting ending by predefined flag

	   {REGFLAG_END_OF_TABLE, 0x00, {}}
	 */
	{0xB9, 3, {0xFF, 0x83, 0x79} },
	{0xB1, 16,
	 {0x44, 0x16, 0x16, 0x31, 0x31, 0x50, 0xD0, 0xEE, 0x54, 0x80, 0x38, 0x38, 0xF8, 0x22, 0x22,
	  0x22} },
	{0xB2, 9, {0x82, 0xFE, 0x0D, 0x0A, 0x20, 0x50, 0x11, 0x42, 0x1D} },
	{0xB4, 10, {0x02, 0x7C, 0x02, 0x7C, 0x02, 0x7C, 0x22, 0x86, 0x23, 0x86} },
	{0xC7, 4, {0x00, 0x00, 0x00, 0xC0} },
	{0xCC, 1, {0x02} },
	{0xD2, 1, {0x11} },
	{0xD3, 29,
	 {0x00, 0x07, 0x00, 0x3C, 0x1C, 0x08, 0x08, 0x32, 0x10, 0x02, 0x00, 0x02, 0x03, 0x70, 0x03,
	  0x70, 0x00, 0x08, 0x00, 0x08, 0x37, 0x33, 0x06, 0x06, 0x37, 0x06, 0x06, 0x37, 0x0B} },
	{0xD5, 34,
	 {0x19, 0x19, 0x18, 0x18, 0x1A, 0x1A, 0x1B, 0x1B, 0x02, 0x03, 0x00, 0x01, 0x06, 0x07, 0x04,
	  0x05, 0x20, 0x21, 0x22, 0x23, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	  0x18, 0x18, 0x00, 0x00} },
	{0xD6, 32,
	 {0x18, 0x18, 0x19, 0x19, 0x1A, 0x1A, 0x1B, 0x1B, 0x03, 0x02, 0x05, 0x04, 0x07, 0x06, 0x01,
	  0x00, 0x23, 0x22, 0x21, 0x20, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	  0x18, 0x18} },
	{0xE0, 42,
	 {0x00, 0x00, 0x04, 0x0B, 0x0D, 0x3F, 0x1A, 0x2D, 0x08, 0x0C, 0x0E, 0x18, 0x10, 0x14, 0x17,
	  0x15, 0x16, 0x09, 0x13, 0x14, 0x18, 0x00, 0x00, 0x04, 0x0B, 0x0D, 0x3F, 0x1A, 0x2E, 0x07,
	  0x0C, 0x0F, 0x19, 0x0F, 0x13, 0x16, 0x14, 0x14, 0x07, 0x12, 0x13, 0x17} },
	{0xB6, 2, {0x48, 0x48} },
	{0x35, 1, {0x00} },
	{0x51, 1, {0xff} },
	{0x53, 1, {0x24} },
	{0x55, 1, {0x00} },
	{0x11, 0, {} },
	{REGFLAG_DELAY, 120, {} },
	{0x29, 0, {} },
	{REGFLAG_DELAY, 40, {} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
#endif
};

static struct LCM_setting_table lcm_backlight_level_setting[] = {
#if ONTIM_MODE
	{DSI_CMD_AUTO, 0, sizeof(lcd_backlight_control_cmd), lcd_backlight_control_cmd},
#else
	{0x51, 2, {0x00, 0x00} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
#endif
};

static void push_table(struct LCM_setting_table *table, unsigned int count,
		       unsigned char force_update)
{
#ifdef ONTIM_MODE
	unsigned int i;

	for (i = 0; i < count; i++) {

		unsigned cmd;
		unsigned char length;
		unsigned char para_list[64];

		switch (table[i].data_type) {

		case REGFLAG_DELAY:
			MDELAY(table[i].delay);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		case DSI_CMD_DCS:
		case DSI_CMD_GEN:
		case DSI_CMD_AUTO:
		default:
			cmd = (unsigned)table[i].data[0];
			if (table[i].length > 1) {
				length = table[i].length - 1;
				if (length > 64)
					memcpy(para_list, table[i].data + 1, 64);
				else
					memcpy(para_list, table[i].data + 1, length);
			} else if (table[i].length == 1)
				length = 0;
			else
				continue;

			if (table[i].data_type == DSI_CMD_DCS)
				dsi_set_cmdq_V2_DCS(cmd, length, para_list, force_update);
			else if (table[i].data_type == DSI_CMD_GEN)
				dsi_set_cmdq_V2_generic(cmd, length, para_list, force_update);
			else
				dsi_set_cmdq_V2(cmd, length, para_list, force_update);
		}
	}
#else
	unsigned int i;

	for (i = 0; i < count; i++) {

		unsigned cmd;

		cmd = table[i].cmd;

		switch (cmd) {

		case REGFLAG_DELAY:
			MDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
#endif
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

	/* enable tearing-free */
	params->dbi.te_mode = LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity = LCM_POLARITY_RISING;

#if defined(LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
#endif

	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_TWO_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	/* Not support in MT6573 */

	/* params->dsi.DSI_WMEM_CONTI=0x3C; */
	/* params->dsi.DSI_RMEM_CONTI=0x3E; */


	params->dsi.packet_size = 256;

	/* Video mode setting */
	/* params->dsi.intermediat_buffer_num = 2; */

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = 4;
	params->dsi.vertical_backporch = 20;
	params->dsi.vertical_frontporch = 12;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 10;
	params->dsi.horizontal_backporch = 60;
	params->dsi.horizontal_frontporch = 60;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	/* Bit rate calculation */
	params->dsi.PLL_CLOCK = 208;

	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0a;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
}

#ifndef ONTIM_MODE
static unsigned int lcm_compare_id(void)
{
	int array[4];
	char buffer[3];
	char id0 = 0;
	char id1 = 0;
	char id2 = 0;

	SET_RESET_PIN(0);
	MDELAY(200);
	SET_RESET_PIN(1);
	MDELAY(200);

	array[0] = 0x00033700;	/* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xDA, buffer, 1);

	array[0] = 0x00033700;	/* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0xDB, buffer + 1, 1);


	array[0] = 0x00033700;	/* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0xDC, buffer + 2, 1);

	id0 = buffer[0];	/* should be 0x00 */
	id1 = buffer[1];	/* should be 0xaa */
	id2 = buffer[2];	/* should be 0x55 */
#ifdef BUILD_LK
	dprintf(0, "%s, id0 = 0x%08x\n", __func__, id0);	/* should be 0x00 */
	dprintf(0, "%s, id1 = 0x%08x\n", __func__, id1);	/* should be 0xaa */
	dprintf(0, "%s, id2 = 0x%08x\n", __func__, id2);	/* should be 0x55 */
#endif

	return 1;
}
#endif
#if 0
static void lcm_init_power(void)
{
#ifdef BUILD_LK
	pmic_set_register_value(PMIC_RG_VGP1_EN, 1);
#else
	hwPowerOn(MT6328_POWER_LDO_VGP1, VOL_2800, "LCM_DRV");
#endif
}

#ifndef ONTIM_MODE
static void lcm_suspend_power(void)
{
#ifdef BUILD_LK
	pmic_set_register_value(PMIC_RG_VGP1_EN, 0);
#else
	hwPowerDown(MT6328_POWER_LDO_VGP1, "LCM_DRV");
#endif
}

static void lcm_resume_power(void)
{
#ifdef BUILD_LK
	pmic_set_register_value(PMIC_RG_VGP1_EN, 1);
#else
	hwPowerOn(MT6328_POWER_LDO_VGP1, VOL_2800, "LCM_DRV");
#endif
}
#endif
#endif
static void lcm_init(void)
{
	pr_debug("kernel %s\n", __func__);
	mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ONE);
	pr_debug("kernel lcm_init1");
	mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ZERO);
	pr_debug("kernel lcm_init2");
	MDELAY(10);
	/* SET_RESET_PIN(1); */
	mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ONE);
	pr_debug("kernel lcm_init3");
	MDELAY(20);
	pr_debug("kernel lcm_init4");

	push_table(lcm_initialization_setting,
		   sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}



static void lcm_suspend(void)
{
	unsigned int data_array[2];

	data_array[0] = 0x00280500;	/* Display Off */
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10);
	data_array[0] = 0x00100500;	/* Sleep In */
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(100);

#ifdef BUILD_LK
	dprintf(0, "uboot %s\n", __func__);
#else
	pr_debug("kernel %s\n", __func__);
#endif
}


static void lcm_resume(void)
{
#ifdef BUILD_LK
	dprintf(0, "uboot %s\n", __func__);
#else
	pr_debug("kernel %s\n", __func__);
#endif
/* push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1); */
	pr_debug("lcm init\n");
	lcm_init();
}


#ifndef ONTIM_MODE
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

#ifdef BUILD_LK
	dprintf(0, "uboot %s\n", __func__);
#else
	pr_debug("kernel %s\n", __func__);
#endif

	data_array[0] = 0x00053902;
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	data_array[3] = 0x00053902;
	data_array[4] = (y1_MSB << 24) | (y0_LSB << 16) | (y0_MSB << 8) | 0x2b;
	data_array[5] = (y1_LSB);
	data_array[6] = 0x002c3909;

	dsi_set_cmdq(data_array, 7, 0);

}
#endif

static void lcm_setbacklight_tcl(unsigned int level)
{

	if (level < 0)
		level = 0;

	else if (level > 255)
		level = 255;
	/* if((level < 12)&&(level > 0)) */
	/* level = 12; */
	/* Refresh value of backlight level. */
#if ONTIM_MODE
	lcm_backlight_level_setting[0].data[1] = level;
#else
	lcm_backlight_level_setting[0].para_list[0] = level;
#endif
	push_table(lcm_backlight_level_setting, ARRAY_SIZE(lcm_backlight_level_setting), 1);
	pr_debug("[kernel]:%s.lcd_backlight_level=%d.\n", __func__, level);
}

static unsigned int lcm_ata_check(unsigned char *buffer)
{
	return 1;
}

LCM_DRIVER ili9806e_dsi_vdo_tcl_blu5039_drv = {
	.name = "ili9806e_dsi_vdo_tcl_blu5039",
	.set_util_funcs = lcm_set_util_funcs,
	/* .compare_id     = lcm_compare_id, */
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.set_backlight = lcm_setbacklight_tcl,
#if defined(LCM_DSI_CMD_MODE)
	.update = lcm_update,
#endif
	.ata_check = lcm_ata_check,
	/* .init_power                = lcm_init_power, */
	/* .resume_power = lcm_resume_power, */
	/* .suspend_power = lcm_suspend_power, */
};
