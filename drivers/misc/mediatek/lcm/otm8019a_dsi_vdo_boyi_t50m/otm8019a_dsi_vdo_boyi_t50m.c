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
#define GPIO_LCD_RST_PIN         (GPIO146 | 0x80000000)
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
static u8 lcd_boyi_otm8019a_cmd_001[] = { 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_002[] = { 0xFF, 0x80, 0x19, 0x01 };
static u8 lcd_boyi_otm8019a_cmd_003[] = { 0x00, 0x80 };
static u8 lcd_boyi_otm8019a_cmd_004[] = { 0xFF, 0x80, 0x19 };
static u8 lcd_boyi_otm8019a_cmd_005[] = { 0x00, 0x90 };
static u8 lcd_boyi_otm8019a_cmd_006[] = { 0xB3, 0x02 };
static u8 lcd_boyi_otm8019a_cmd_007[] = { 0x00, 0x92 };
static u8 lcd_boyi_otm8019a_cmd_008[] = { 0xB3, 0x45 };
static u8 lcd_boyi_otm8019a_cmd_009[] = { 0x00, 0x80 };
static u8 lcd_boyi_otm8019a_cmd_010[] = {
	0xC0, 0x00, 0x58, 0x00, 0x15, 0x15, 0x00, 0x58, 0x15, 0x15 };
static u8 lcd_boyi_otm8019a_cmd_011[] = { 0x00, 0x90 };
static u8 lcd_boyi_otm8019a_cmd_012[] = { 0xC0, 0x00, 0x15, 0x00, 0x00, 0x00, 0x03 };
static u8 lcd_boyi_otm8019a_cmd_013[] = { 0x00, 0xA2 };
static u8 lcd_boyi_otm8019a_cmd_014[] = { 0xC0, 0x02, 0x1B, 0x02 };
static u8 lcd_boyi_otm8019a_cmd_015[] = { 0x00, 0xB4 };
static u8 lcd_boyi_otm8019a_cmd_016[] = { 0xC0, 0x00 };	/* 77 */
static u8 lcd_boyi_otm8019a_cmd_017[] = { 0x00, 0x89 };
static u8 lcd_boyi_otm8019a_cmd_018[] = { 0xC4, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_019[] = { 0x00, 0x81 };
static u8 lcd_boyi_otm8019a_cmd_020[] = { 0xC4, 0x83 };
static u8 lcd_boyi_otm8019a_cmd_021[] = { 0x00, 0x81 };
static u8 lcd_boyi_otm8019a_cmd_022[] = { 0xC5, 0x66 };
static u8 lcd_boyi_otm8019a_cmd_023[] = { 0x00, 0x82 };
static u8 lcd_boyi_otm8019a_cmd_024[] = { 0xC5, 0xB0 };
static u8 lcd_boyi_otm8019a_cmd_025[] = { 0x00, 0x90 };
static u8 lcd_boyi_otm8019a_cmd_026[] = { 0xC5, 0x6E, 0x79, 0x01, 0x33, 0x44, 0x44 };
static u8 lcd_boyi_otm8019a_cmd_027[] = { 0x00, 0xB1 };
static u8 lcd_boyi_otm8019a_cmd_028[] = { 0xC5, 0xA9 };
static u8 lcd_boyi_otm8019a_cmd_029[] = { 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_030[] = { 0xD8, 0x6F, 0x6F };	/* 62 */
static u8 lcd_boyi_otm8019a_cmd_031[] = { 0x00, 0x80 };
static u8 lcd_boyi_otm8019a_cmd_032[] = {
	0xCE, 0x8B, 0x03, 0x00, 0x8A, 0x03, 0x00, 0x89, 0x03, 0x00, 0x88, 0x03, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_033[] = { 0x00, 0xA0 };
static u8 lcd_boyi_otm8019a_cmd_034[] = {
	0xCE, 0x38, 0x07, 0x03, 0x54, 0x00, 0x00, 0x00, 0x38, 0x06, 0x03, 0x55, 0x00, 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_035[] = { 0x00, 0xB0 };
static u8 lcd_boyi_otm8019a_cmd_036[] = {
	0xCE, 0x38, 0x05, 0x03, 0x56, 0x00, 0x00, 0x00, 0x38, 0x04, 0x03, 0x57, 0x00, 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_037[] = { 0x00, 0xC0 };
static u8 lcd_boyi_otm8019a_cmd_038[] = {
	0xCE, 0x38, 0x03, 0x03, 0x58, 0x00, 0x00, 0x00, 0x38, 0x02, 0x03, 0x59, 0x00, 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_039[] = { 0x00, 0xD0 };
static u8 lcd_boyi_otm8019a_cmd_040[] = {
	0xCE, 0x38, 0x01, 0x03, 0x5A, 0x00, 0x00, 0x00, 0x38, 0x00, 0x03, 0x5B, 0x00, 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_041[] = { 0x00, 0xC0 };
static u8 lcd_boyi_otm8019a_cmd_042[] = {
	0xCF, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_043[] = { 0x00, 0x80 };
static u8 lcd_boyi_otm8019a_cmd_044[] = {
	0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_045[] = { 0x00, 0x90 };
static u8 lcd_boyi_otm8019a_cmd_046[] = {
	0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_047[] = { 0x00, 0xA0 };
static u8 lcd_boyi_otm8019a_cmd_048[] = { 0xCB, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_049[] = { 0x00, 0xA5 };
static u8 lcd_boyi_otm8019a_cmd_050[] = {
	0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_051[] = { 0x00, 0xB0 };
static u8 lcd_boyi_otm8019a_cmd_052[] = { 0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_053[] = { 0x00, 0xC0 };
static u8 lcd_boyi_otm8019a_cmd_054[] = {
	0xCB, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00 };
static u8 lcd_boyi_otm8019a_cmd_055[] = { 0x00, 0xD0 };
static u8 lcd_boyi_otm8019a_cmd_056[] = { 0xCB, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_057[] = { 0x00, 0xd5 };
static u8 lcd_boyi_otm8019a_cmd_058[] = {
	0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01 };
static u8 lcd_boyi_otm8019a_cmd_059[] = { 0x00, 0xE0 };
static u8 lcd_boyi_otm8019a_cmd_060[] = { 0xCB, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
static u8 lcd_boyi_otm8019a_cmd_061[] = { 0x00, 0x80 };
static u8 lcd_boyi_otm8019a_cmd_062[] = {
	0xCC, 0x26, 0x25, 0x21, 0x22, 0x0C, 0x0A, 0x10, 0x0E, 0x02, 0x04 };
static u8 lcd_boyi_otm8019a_cmd_063[] = { 0x00, 0x90 };
static u8 lcd_boyi_otm8019a_cmd_064[] = { 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_065[] = { 0x00, 0x9A };
static u8 lcd_boyi_otm8019a_cmd_066[] = { 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_067[] = { 0x00, 0xA0 };
static u8 lcd_boyi_otm8019a_cmd_068[] = {
	0xCC, 0x00, 0x03, 0x01, 0x0d, 0x0f, 0x09, 0x0b, 0x22, 0x21, 0x25, 0x26 };
static u8 lcd_boyi_otm8019a_cmd_069[] = { 0x00, 0xB0 };
static u8 lcd_boyi_otm8019a_cmd_070[] = {
	0xCC, 0x25, 0x26, 0x21, 0x22, 0x10, 0x0a, 0x0c, 0x0e, 0x04, 0x02 };
static u8 lcd_boyi_otm8019a_cmd_071[] = { 0x00, 0xC0 };
static u8 lcd_boyi_otm8019a_cmd_072[] = { 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_073[] = { 0x00, 0xCA };
static u8 lcd_boyi_otm8019a_cmd_074[] = { 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_075[] = { 0x00, 0xD0 };
static u8 lcd_boyi_otm8019a_cmd_076[] = {
	0xCC, 0x00, 0x01, 0x03, 0x0d, 0x0b, 0x09, 0x0f, 0x22, 0x21, 0x26, 0x25 };
static u8 lcd_boyi_otm8019a_cmd_077[] = { 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_078[] = {
	0xE1, 0x06, 0x07, 0x0B, 0x17, 0x2B, 0x42, 0x4F, 0x88, 0x79, 0x8F, 0x79, 0x69, 0x82, 0x6F,
	0x77, 0x71, 0x6A, 0x63, 0x5A, 0x05 };
static u8 lcd_boyi_otm8019a_cmd_079[] = { 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_080[] = {
	0xE2, 0x06, 0x07, 0x0B, 0x17, 0x2B, 0x42, 0x4F, 0x88, 0x79, 0x8F, 0x79, 0x69, 0x82, 0x6F,
	0x77, 0x71, 0x6A, 0x63, 0x5A, 0x05 };
static u8 lcd_boyi_otm8019a_cmd_081[] = { 0x00, 0x80 };
static u8 lcd_boyi_otm8019a_cmd_082[] = { 0xC4, 0x30 };
static u8 lcd_boyi_otm8019a_cmd_083[] = { 0x00, 0x81 };
static u8 lcd_boyi_otm8019a_cmd_084[] = { 0xC4, 0x86 };
static u8 lcd_boyi_otm8019a_cmd_085[] = { 0x00, 0x8D };
static u8 lcd_boyi_otm8019a_cmd_086[] = { 0xC4, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_087[] = { 0x00, 0x98 };
static u8 lcd_boyi_otm8019a_cmd_088[] = { 0xC0, 0x00 };	/*  */
static u8 lcd_boyi_otm8019a_cmd_089[] = { 0x00, 0xa9 };
static u8 lcd_boyi_otm8019a_cmd_090[] = { 0xC0, 0x0A };	/* 0x06 */
static u8 lcd_boyi_otm8019a_cmd_091[] = { 0x00, 0xb0 };
static u8 lcd_boyi_otm8019a_cmd_092[] = { 0xC1, 0x20, 0x00, 0x00 };	/*  */
static u8 lcd_boyi_otm8019a_cmd_093[] = { 0x00, 0xe1 };
static u8 lcd_boyi_otm8019a_cmd_094[] = { 0xC0, 0x40, 0x30 };	/* 0x40,0x18 */
static u8 lcd_boyi_otm8019a_cmd_095[] = { 0x00, 0x80 };
static u8 lcd_boyi_otm8019a_cmd_096[] = { 0xC1, 0x03, 0x33 };
static u8 lcd_boyi_otm8019a_cmd_097[] = { 0x00, 0xA0 };
static u8 lcd_boyi_otm8019a_cmd_098[] = { 0xC1, 0xe8 };
static u8 lcd_boyi_otm8019a_cmd_099[] = { 0x00, 0x90 };
static u8 lcd_boyi_otm8019a_cmd_100[] = { 0xb6, 0xb4 };	/* command fial            Delay---10 */
static u8 lcd_boyi_otm8019a_cmd_101[] = { 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_102[] = { 0xfb, 0x01 };
static u8 lcd_boyi_otm8019a_cmd_103[] = { 0x00, 0x80 };
static u8 lcd_boyi_otm8019a_cmd_104[] = { 0xC4, 0x30 };
static u8 lcd_boyi_otm8019a_cmd_105[] = { 0x00, 0x98 };
static u8 lcd_boyi_otm8019a_cmd_106[] = { 0xC0, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_107[] = { 0x00, 0xa9 };
static u8 lcd_boyi_otm8019a_cmd_108[] = { 0xC0, 0x0A };
static u8 lcd_boyi_otm8019a_cmd_109[] = { 0x00, 0xb0 };
static u8 lcd_boyi_otm8019a_cmd_110[] = { 0xC1, 0x20, 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_111[] = { 0x00, 0xe1 };
static u8 lcd_boyi_otm8019a_cmd_112[] = { 0xC0, 0x40, 0x30 };
static u8 lcd_boyi_otm8019a_cmd_113[] = { 0x00, 0x80 };
static u8 lcd_boyi_otm8019a_cmd_114[] = { 0xC1, 0x03, 0x33 };
static u8 lcd_boyi_otm8019a_cmd_115[] = { 0x00, 0xA0 };
static u8 lcd_boyi_otm8019a_cmd_116[] = { 0xC1, 0xe8 };
static u8 lcd_boyi_otm8019a_cmd_117[] = { 0x00, 0x90 };
static u8 lcd_boyi_otm8019a_cmd_118[] = { 0xb6, 0xb4 };	/* Delay---10 */
static u8 lcd_boyi_otm8019a_cmd_119[] = { 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_120[] = { 0xfb, 0x01 };
static u8 lcd_boyi_otm8019a_cmd_121[] = { 0x00, 0xA7 };
static u8 lcd_boyi_otm8019a_cmd_122[] = { 0xB3, 0x10 };	/* Delay---10 */
static u8 lcd_boyi_otm8019a_cmd_123[] = { 0x00, 0xA1 };
static u8 lcd_boyi_otm8019a_cmd_124[] = { 0xB3, 0x00 };

static u8 lcd_boyi_otm8019a_cmd_125[] = { 0x35, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_126[] = { 0x51, 0x00 };	/* hzb for test 0x00 */
static u8 lcd_boyi_otm8019a_cmd_127[] = { 0x53, 0x24 };
static u8 lcd_boyi_otm8019a_cmd_128[] = { 0x55, 0x00 };

static u8 lcd_boyi_otm8019a_cmd_129[] = { 0x00, 0x00 };
static u8 lcd_boyi_otm8019a_cmd_130[] = { 0xFF, 0xFF, 0xFF, 0xFF };

static u8 lcd_backlight_control_cmd[] = { 0x51, 0x00 };
#endif


static struct LCM_setting_table lcm_initialization_setting[] = {
#if ONTIM_MODE
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_001), lcd_boyi_otm8019a_cmd_001},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_002), lcd_boyi_otm8019a_cmd_002},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_003), lcd_boyi_otm8019a_cmd_003},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_004), lcd_boyi_otm8019a_cmd_004},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_005), lcd_boyi_otm8019a_cmd_005},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_006), lcd_boyi_otm8019a_cmd_006},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_007), lcd_boyi_otm8019a_cmd_007},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_008), lcd_boyi_otm8019a_cmd_008},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_009), lcd_boyi_otm8019a_cmd_009},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_010), lcd_boyi_otm8019a_cmd_010},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_011), lcd_boyi_otm8019a_cmd_011},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_012), lcd_boyi_otm8019a_cmd_012},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_013), lcd_boyi_otm8019a_cmd_013},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_014), lcd_boyi_otm8019a_cmd_014},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_015), lcd_boyi_otm8019a_cmd_015},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_016), lcd_boyi_otm8019a_cmd_016},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_017), lcd_boyi_otm8019a_cmd_017},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_018), lcd_boyi_otm8019a_cmd_018},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_019), lcd_boyi_otm8019a_cmd_019},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_020), lcd_boyi_otm8019a_cmd_020},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_021), lcd_boyi_otm8019a_cmd_021},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_022), lcd_boyi_otm8019a_cmd_022},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_023), lcd_boyi_otm8019a_cmd_023},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_024), lcd_boyi_otm8019a_cmd_024},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_025), lcd_boyi_otm8019a_cmd_025},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_026), lcd_boyi_otm8019a_cmd_026},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_027), lcd_boyi_otm8019a_cmd_027},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_028), lcd_boyi_otm8019a_cmd_028},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_029), lcd_boyi_otm8019a_cmd_029},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_030), lcd_boyi_otm8019a_cmd_030},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_031), lcd_boyi_otm8019a_cmd_031},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_032), lcd_boyi_otm8019a_cmd_032},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_033), lcd_boyi_otm8019a_cmd_033},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_034), lcd_boyi_otm8019a_cmd_034},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_035), lcd_boyi_otm8019a_cmd_035},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_036), lcd_boyi_otm8019a_cmd_036},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_037), lcd_boyi_otm8019a_cmd_037},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_038), lcd_boyi_otm8019a_cmd_038},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_039), lcd_boyi_otm8019a_cmd_039},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_040), lcd_boyi_otm8019a_cmd_040},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_041), lcd_boyi_otm8019a_cmd_041},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_042), lcd_boyi_otm8019a_cmd_042},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_043), lcd_boyi_otm8019a_cmd_043},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_044), lcd_boyi_otm8019a_cmd_044},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_045), lcd_boyi_otm8019a_cmd_045},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_046), lcd_boyi_otm8019a_cmd_046},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_047), lcd_boyi_otm8019a_cmd_047},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_048), lcd_boyi_otm8019a_cmd_048},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_049), lcd_boyi_otm8019a_cmd_049},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_050), lcd_boyi_otm8019a_cmd_050},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_051), lcd_boyi_otm8019a_cmd_051},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_052), lcd_boyi_otm8019a_cmd_052},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_053), lcd_boyi_otm8019a_cmd_053},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_054), lcd_boyi_otm8019a_cmd_054},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_055), lcd_boyi_otm8019a_cmd_055},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_056), lcd_boyi_otm8019a_cmd_056},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_057), lcd_boyi_otm8019a_cmd_057},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_058), lcd_boyi_otm8019a_cmd_058},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_059), lcd_boyi_otm8019a_cmd_059},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_060), lcd_boyi_otm8019a_cmd_060},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_061), lcd_boyi_otm8019a_cmd_061},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_062), lcd_boyi_otm8019a_cmd_062},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_063), lcd_boyi_otm8019a_cmd_063},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_064), lcd_boyi_otm8019a_cmd_064},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_065), lcd_boyi_otm8019a_cmd_065},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_066), lcd_boyi_otm8019a_cmd_066},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_067), lcd_boyi_otm8019a_cmd_067},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_068), lcd_boyi_otm8019a_cmd_068},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_069), lcd_boyi_otm8019a_cmd_069},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_070), lcd_boyi_otm8019a_cmd_070},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_071), lcd_boyi_otm8019a_cmd_071},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_072), lcd_boyi_otm8019a_cmd_072},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_073), lcd_boyi_otm8019a_cmd_073},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_074), lcd_boyi_otm8019a_cmd_074},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_075), lcd_boyi_otm8019a_cmd_075},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_076), lcd_boyi_otm8019a_cmd_076},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_077), lcd_boyi_otm8019a_cmd_077},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_078), lcd_boyi_otm8019a_cmd_078},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_079), lcd_boyi_otm8019a_cmd_079},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_080), lcd_boyi_otm8019a_cmd_080},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_081), lcd_boyi_otm8019a_cmd_081},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_082), lcd_boyi_otm8019a_cmd_082},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_083), lcd_boyi_otm8019a_cmd_083},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_084), lcd_boyi_otm8019a_cmd_084},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_085), lcd_boyi_otm8019a_cmd_085},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_086), lcd_boyi_otm8019a_cmd_086},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_087), lcd_boyi_otm8019a_cmd_087},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_088), lcd_boyi_otm8019a_cmd_088},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_089), lcd_boyi_otm8019a_cmd_089},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_090), lcd_boyi_otm8019a_cmd_090},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_091), lcd_boyi_otm8019a_cmd_091},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_092), lcd_boyi_otm8019a_cmd_092},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_093), lcd_boyi_otm8019a_cmd_093},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_094), lcd_boyi_otm8019a_cmd_094},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_095), lcd_boyi_otm8019a_cmd_095},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_096), lcd_boyi_otm8019a_cmd_096},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_097), lcd_boyi_otm8019a_cmd_097},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_098), lcd_boyi_otm8019a_cmd_098},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_099), lcd_boyi_otm8019a_cmd_099},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_100), lcd_boyi_otm8019a_cmd_100},
	{REGFLAG_DELAY, 10, 0, NULL},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_101), lcd_boyi_otm8019a_cmd_101},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_102), lcd_boyi_otm8019a_cmd_102},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_103), lcd_boyi_otm8019a_cmd_103},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_104), lcd_boyi_otm8019a_cmd_104},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_105), lcd_boyi_otm8019a_cmd_105},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_106), lcd_boyi_otm8019a_cmd_106},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_107), lcd_boyi_otm8019a_cmd_107},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_108), lcd_boyi_otm8019a_cmd_108},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_109), lcd_boyi_otm8019a_cmd_109},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_110), lcd_boyi_otm8019a_cmd_110},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_111), lcd_boyi_otm8019a_cmd_111},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_112), lcd_boyi_otm8019a_cmd_112},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_113), lcd_boyi_otm8019a_cmd_113},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_114), lcd_boyi_otm8019a_cmd_114},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_115), lcd_boyi_otm8019a_cmd_115},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_116), lcd_boyi_otm8019a_cmd_116},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_117), lcd_boyi_otm8019a_cmd_117},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_118), lcd_boyi_otm8019a_cmd_118},
	{REGFLAG_DELAY, 10, 0, NULL},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_119), lcd_boyi_otm8019a_cmd_119},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_120), lcd_boyi_otm8019a_cmd_120},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_121), lcd_boyi_otm8019a_cmd_121},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_122), lcd_boyi_otm8019a_cmd_122},
	{REGFLAG_DELAY, 10, 0, NULL},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_123), lcd_boyi_otm8019a_cmd_123},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_124), lcd_boyi_otm8019a_cmd_124},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_125), lcd_boyi_otm8019a_cmd_125},
	{DSI_CMD_GEN, 0, sizeof(lcd_boyi_otm8019a_cmd_126), lcd_boyi_otm8019a_cmd_126},
	{DSI_CMD_DCS, 0, sizeof(lcd_boyi_otm8019a_cmd_127), lcd_boyi_otm8019a_cmd_127},
	{DSI_CMD_DCS, 0, sizeof(lcd_boyi_otm8019a_cmd_128), lcd_boyi_otm8019a_cmd_128},
	{DSI_CMD_DCS, 0, sizeof(lcd_boyi_otm8019a_cmd_129), lcd_boyi_otm8019a_cmd_129},
	{DSI_CMD_DCS, 0, sizeof(lcd_boyi_otm8019a_cmd_130), lcd_boyi_otm8019a_cmd_130},
	{DSI_CMD_DCS, 0, sizeof(exit_sleep), exit_sleep},
	{REGFLAG_DELAY, 120, 0, NULL},
	{DSI_CMD_DCS, 0, sizeof(display_on), display_on},
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
	{DSI_CMD_AUTO, 0, sizeof(lcd_backlight_control_cmd), lcd_backlight_control_cmd}	,
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
	params->dsi.lcm_esd_check_table[1].cmd = 0x0d;
	params->dsi.lcm_esd_check_table[1].count = 1;
	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x00;
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
	/* mt_set_gpio_mode(GPIO_LCD_RST_PIN,GPIO_MODE_GPIO); */
	/* MDELAY(10); */
	mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ONE);
	mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ZERO);
	MDELAY(10);
	/* SET_RESET_PIN(1); */
	mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ONE);
	MDELAY(20);

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

static void lcm_setbacklight_boyi(unsigned int level)
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

LCM_DRIVER otm8019a_dsi_vdo_boyi_t50m_drv = {
	.name = "otm8019a_dsi_vdo_boyi_t50m",
	.set_util_funcs = lcm_set_util_funcs,
	/* .compare_id     = lcm_compare_id, */
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.set_backlight = lcm_setbacklight_boyi,
#if defined(LCM_DSI_CMD_MODE)
	.update = lcm_update,
#endif
	.ata_check = lcm_ata_check,
	/* .init_power                = lcm_init_power, */
	/* .resume_power = lcm_resume_power, */
	/* .suspend_power = lcm_suspend_power, */
};
