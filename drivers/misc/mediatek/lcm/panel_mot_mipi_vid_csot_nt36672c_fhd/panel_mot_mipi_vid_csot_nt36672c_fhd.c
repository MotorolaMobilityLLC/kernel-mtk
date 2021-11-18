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

#include "mtkfb_params.h"

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

#define LCM_ID_NT36672C_TIANMA (0x03)

#define LCM_BL_DRV_I2C_SUPPORT
#define LCM_BL_CMD_LP_MODE

#define PWM_51_POS	2
static bool bl_lm3697 = false;
static bool config_pwm = false;
static bool first_bl_check = true;
static struct LCM_UTIL_FUNCS lcm_util;
#ifndef LCM_BL_DRV_I2C_SUPPORT
extern char BL_control_read_bytes(unsigned char cmd);
extern int BL_control_write_bytes(unsigned char addr, unsigned char value);
#endif
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
	{0x10, 0, {} },
	{REGFLAG_DELAY, 60, {} },
};

static struct LCM_setting_table init_setting[] = {
	{0xFF,0x01,{0x10}},
	{0xFB,0x01,{0x01}},
	{0X51,0x02,{0x0C, 0xEC}},   // Do not move 51 pos alone, should keep same with PWM_51_POS
	{0X53,0x01,{0X2C}},
	{0X55,0x01,{0X01}},
	{0X36,0x01,{0X00}},
	{0X3B,0x05,{0X03,0X14,0X36,0X04,0X04}},
	{0xB0,0x01,{0x00}},
	{0XC0,0x01,{0X03}},
	{0XC1,0x10,{0X89,0X28,0x00,0x14,0x00,0xAA,0x02,0x0E,0x00,0x71,0x00,0x07,0x05,0x0E,0x05,0x16}},
	{0xC2,0x02,{0x1B,0xA0}},
	{0xFF,0x01,{0x24}},
	{0xFB,0x01,{0x01}},
	{0xFF,0x01,{0x25}},
	{0xFB,0x01,{0x01}},
	{0x18,0x01,{0x21}},
	{0xFF,0x01,{0xE0}},
	{0xFB,0x01,{0x01}},
	{0x35,0x01,{0x82}},
	{0xFF,0x01,{0xF0}},
	{0xFB,0x01,{0x01}},
	{0x1C,0x01,{0x01}},
	{0x33,0x01,{0x01}},
	{0x5A,0x01,{0x00}},
	{0xD2,0x01,{0x50}},
	{0xFF,0x01,{0xD0}},
	{0xFB,0x01,{0x01}},
	{0x53,0x01,{0x22}},
	{0x54,0x01,{0x02}},
	{0xFF,0x01,{0xC0}},
	{0xFB,0x01,{0x01}},
	{0x9C,0x01,{0x11}},
	{0x9D,0x01,{0x11}},
	{0xFF,0x01,{0x23}},
	{0xFB,0x01,{0x01}},
	{0X00,0x01,{0X80}},//12bit
	{0X07,0x01,{0X00}},//20KHZ
	{0X08,0x01,{0X01}},//20KHZ
	{0X09,0x01,{0X55}},//20KHZ
	{0XFF,0x01,{0X10}},
	{0XFB,0x01,{0X01}},

	{0X11,0x00,{}},
	{REGFLAG_DELAY,70,{}},
	{0X29,0x00,{}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
/*
static struct LCM_setting_table bl_level[] = {
	{0x51, 1, {0xFF} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};
*/
#ifndef LCM_BL_DRV_I2C_SUPPORT
static struct LCM_setting_table_V4 BL_Level[] = {
	{0x15, 0x51, 1, {0xFF}, 0 },
};
#endif

#ifdef LCM_BL_CMD_LP_MODE
static struct LCM_setting_table_V4 lcm_hbm_on[] = {
	{0x39, 0x51, 2, {0X0F, 0xFF}, 0 },
};

static struct LCM_setting_table_V4 lcm_hbm_off[] = {
	{0x39, 0x51, 2, {0x0C, 0xEC}, 0 },
};

static struct LCM_setting_table_V4 lcm_hbm_off_lm3697[] = {
	{0x39, 0x51, 2, {0x0C, 0xFC}, 0 },
};

static struct LCM_setting_table_V4 lcm_cabc_ui[] = {
	{0x15, 0x55, 1, {0X01}, 0 },
};

static struct LCM_setting_table_V4 lcm_cabc_mv[] = {
	{0x15, 0x55, 1, {0X03}, 0 },
};

static struct LCM_setting_table_V4 lcm_cabc_dis[] = {
	{0x15, 0x55, 1, {0X00}, 0 },
};

#else
static struct LCM_setting_table lcm_cabc_setting[] = {
	{0x55, 1, {0x01} },	//UI
	{0x55, 1, {0x03} },	//MV
	{0x55, 1, {0x00} },	//DISABLE
};

static struct LCM_setting_table lcm_hbm_setting[] = {
	{0x51, 2, {0x0C, 0XCC} },	//80% PWM
	{0x51, 2, {0x0F, 0XFF} },	//100% PWM
};
#endif

static void push_table(void *cmdq, struct LCM_setting_table *table,
		       unsigned int count, unsigned char force_update)
{
	unsigned int i;
	unsigned int cmd;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;

		switch (cmd) {

		case REGFLAG_DELAY:
				MDELAY(table[i].count);
			break;

		case REGFLAG_UDELAY:
			UDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
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

	pr_debug("%s enter\n", __func__);
	dsi->dfps_enable = 1;
	dsi->dfps_default_fps = 9000;/*real fps * 100, to support float*/
	dsi->dfps_def_vact_tim_fps = 9000;/*real vact timing fps * 100*/

	/* DPFS_LEVEL0 */
	dfps_params[0].level = DFPS_LEVEL0;
	dfps_params[0].fps = 9000;/*real fps * 100, to support float*/
	dfps_params[0].vact_timing_fps = 9000;/*real vact timing fps * 100*/
	dfps_params[0].vertical_frontporch = 54;  //90Hz
	dfps_params[0].vertical_frontporch_for_low_power = 0;

	/* DPFS_LEVEL1 */
	dfps_params[1].level = DFPS_LEVEL1;
	dfps_params[1].fps = 6000;/*real fps * 100, to support float*/
	dfps_params[1].vact_timing_fps = 9000;/*real vact timing fps * 100*/
	dfps_params[1].vertical_frontporch = 1321; //60Hz
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

	params->dsi.vertical_sync_active = 2;
	params->dsi.vertical_backporch = 18;
	params->dsi.vertical_frontporch = 54;			//90HZ
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 4;
	params->dsi.horizontal_backporch = 18;
	params->dsi.horizontal_frontporch = 200;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_disable = 1;
	params->dsi.dsc_enable = 0;

	params->dsi.bdg_dsc_enable = 1;
	params->dsi.bdg_ssc_disable = 1;
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

	params->dsi.clk_lp_per_line_enable = 0;

	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
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

static void lcm_bl_ic_config() {
	first_bl_check = false;
	if (saved_command_line) {
		char *sub;
		char lm3697_key[] = "LM3697_EX";

		sub = strstr(saved_command_line, lm3697_key);
		if (sub) {
			bl_lm3697 = true;
			pr_info("%s:lm3697 match\n", __func__);

			//reconfig 51 cmd on init_setting
			if (config_pwm && (0x51 == init_setting[PWM_51_POS].cmd)) {
				//keep consistent with lcm_hbm_off_lm3697
				init_setting[PWM_51_POS].para_list[0] = lcm_hbm_off_lm3697[0].para_list[0];
				init_setting[PWM_51_POS].para_list[1] = lcm_hbm_off_lm3697[0].para_list[1];

				pr_info("%s:lm3697:new init_setting[%d].para_list[0]=0x%02x\n", __func__, PWM_51_POS, init_setting[PWM_51_POS].para_list[0]);
				pr_info("%s:lm3697:new init_setting[%d].para_list[1]=0x%02x\n", __func__, PWM_51_POS, init_setting[PWM_51_POS].para_list[1]);
			}
			else
				pr_err("%s:lm3697:config_pwm:%d, init_setting[%d].cmd=0x%02x\n", __func__, config_pwm, PWM_51_POS, init_setting[PWM_51_POS].cmd);
		}
	}
	else
		pr_info("%s: saved_command_line NULL\n", __func__);

}

static void lcm_init(void)
{
	pr_info("%s enter\n", __func__);

	SET_RESET_PIN(0);
	MDELAY(1);

	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BIAS_ENP_OUT1);
	Bias_power_write_bytes(0x00,0x0F);
	MDELAY(1);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BIAS_ENN_OUT1);
	Bias_power_write_bytes(0x01,0x0F);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(5);
	SET_RESET_PIN(0);
	MDELAY(5);
	SET_RESET_PIN(1);
	MDELAY(10);

	if (config_pwm && first_bl_check) {
		lcm_bl_ic_config();
	}

	if (bl_lm3697 && (0x51 == init_setting[PWM_51_POS].cmd)) {
		pr_info("%s:lm3697:new init_setting[%d].para_list[0]=0x%02x\n", __func__, PWM_51_POS, init_setting[PWM_51_POS].para_list[0]);
		pr_info("%s:lm3697:new init_setting[%d].para_list[1]=0x%02x\n", __func__, PWM_51_POS, init_setting[PWM_51_POS].para_list[1]);
	}
	else {
		pr_info("%s:init_setting[%d].para_list[0]=0x%02x\n", __func__, PWM_51_POS, init_setting[PWM_51_POS].para_list[0]);
		pr_info("%s:init_setting[%d].para_list[1]=0x%02x\n", __func__, PWM_51_POS, init_setting[PWM_51_POS].para_list[1]);
	}

	push_table(NULL, init_setting,
		sizeof(init_setting)/sizeof(struct LCM_setting_table), 1);

	MDELAY(30);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BL_EN_OUT1);

#ifndef LCM_BL_DRV_I2C_SUPPORT
	pr_info("%s read bl reg 00h = 0x%x\n",__func__,BL_control_read_bytes(0x00));
	if(0x03 == BL_control_read_bytes(0x00))
		BL_control_write_bytes(0x02,0x01);
	else{
		BL_control_write_bytes(0x10,0x07);
		BL_control_write_bytes(0x16,0x01);
		BL_control_write_bytes(0x19,0x07);
		BL_control_write_bytes(0x1A,0x04);
		BL_control_write_bytes(0x1C,0x0E);
		BL_control_write_bytes(0x24,0x02);
		BL_control_write_bytes(0x22,0x0F);
		BL_control_write_bytes(0x23,0xFF);
	}
#endif

	pr_info("%s end\n", __func__);

}

static void lcm_suspend(void)
{
	pr_info("%s enter\n", __func__);

	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BL_EN_OUT0);

	push_table(NULL, lcm_suspend_setting,
		sizeof(lcm_suspend_setting)/sizeof(struct LCM_setting_table),
		1);

	pr_info("%s end\n", __func__);

}

static void lcm_suspend_power(void)
{
	pr_info("%s enter\n", __func__);

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

#ifdef LCM_BL_DRV_I2C_SUPPORT
static void lcm_setbacklight(unsigned int level)
{
	pr_info("%s,csot_nt36672c enter: level=%d\n", __func__, level);
	return;
}
#else
static void lcm_setbacklight_cmdq(void *handle,unsigned int level)
{

	pr_info("%s,nt36672c huaxing backlight: level = %d\n", __func__, level);

	if(level < 3 && level != 0)
		level = 3;
	BL_Level[0].para_list[0] = level;

	dsi_set_cmdq_V4(BL_Level,
				sizeof(BL_Level)
				/ sizeof(struct LCM_setting_table_V4), 1);

}
#endif

static void *lcm_switch_mode(int mode)
{
	return NULL;
}

static void lcm_set_cmdq(void *handle, unsigned int *lcm_cmd,
		unsigned int *lcm_count, unsigned int *lcm_value)
{
	switch(*lcm_cmd) {
		case PARAM_HBM:
#ifdef LCM_BL_CMD_LP_MODE
			if (*lcm_value) {
				dsi_set_cmdq_V4(lcm_hbm_on,
						sizeof(lcm_hbm_on)/sizeof(struct LCM_setting_table_V4), 1);

				//TBD, in lp mode with 2 bit, it need send cmd twice to make it work currently
				MDELAY(5);
				dsi_set_cmdq_V4(lcm_hbm_on,
						sizeof(lcm_hbm_on)/sizeof(struct LCM_setting_table_V4), 1);

				pr_debug("%s, csot_nt36672c HBM on\n", __func__);
			}
			else {
				if (config_pwm && bl_lm3697) {
					pr_info("%s: set lm3697 hbm off\n", __func__);
					dsi_set_cmdq_V4(lcm_hbm_off_lm3697,
							sizeof(lcm_hbm_off)/sizeof(struct LCM_setting_table_V4), 1);
					//TBD
					MDELAY(5);
					dsi_set_cmdq_V4(lcm_hbm_off_lm3697,
							sizeof(lcm_hbm_off)/sizeof(struct LCM_setting_table_V4), 1);
				}
				else {
					pr_info("%s: set aw99703 hbm off\n", __func__);
					dsi_set_cmdq_V4(lcm_hbm_off,
							sizeof(lcm_hbm_off)/sizeof(struct LCM_setting_table_V4), 1);
					//TBD
					MDELAY(5);
					dsi_set_cmdq_V4(lcm_hbm_off,
							sizeof(lcm_hbm_off)/sizeof(struct LCM_setting_table_V4), 1);
				}
			}
#else
			pr_debug("%s:push lcm_hbm_setting:%d", __func__, *lcm_value);
			push_table(handle, &lcm_hbm_setting[*lcm_value], 1, 1);
#endif
			pr_info("%s, csot_nt36672c set HBM %d\n", __func__, *lcm_value);
			break;
		case PARAM_CABC:
#ifdef LCM_BL_CMD_LP_MODE
			switch(*lcm_value) {
				case CABC_UI_MODE:
					dsi_set_cmdq_V4(lcm_cabc_ui,
						sizeof(lcm_cabc_ui)/sizeof(struct LCM_setting_table_V4), 1);
					break;
				case CABC_MV_MODE:
					dsi_set_cmdq_V4(lcm_cabc_mv,
						sizeof(lcm_cabc_mv)/sizeof(struct LCM_setting_table_V4), 1);
					break;
				default:
					dsi_set_cmdq_V4(lcm_cabc_dis,
						sizeof(lcm_cabc_dis)/sizeof(struct LCM_setting_table_V4), 1);
					break;
			}
#else
			pr_debug("%s:push lcm_cabc_setting:%d", __func__, *lcm_value);
			push_table(handle, &lcm_cabc_setting[*lcm_value], 1, 1);
#endif
			pr_info("%s, csot_nt36672c set cabc %d\n", __func__, *lcm_value);

			if (first_bl_check && config_pwm) {
				if (first_bl_check)
					lcm_bl_ic_config();

				if (bl_lm3697) {
					MDELAY(5);
					dsi_set_cmdq_V4(lcm_hbm_off_lm3697,
							sizeof(lcm_hbm_off)/sizeof(struct LCM_setting_table_V4), 1);
					//TBD
					MDELAY(5);
					dsi_set_cmdq_V4(lcm_hbm_off_lm3697,
							sizeof(lcm_hbm_off)/sizeof(struct LCM_setting_table_V4), 1);

					pr_info("%s: config lm3697 init pwm done\n", __func__);
				}
				else
					pr_info("%s: not lm3697\n", __func__);
			}

			break;
		default:
			pr_err("%s,csot_nt36672c cmd:%d, unsupport\n", __func__, *lcm_cmd);
			break;
	}

}

#ifdef CONFIG_LCM_NOTIFIY_SUPPORT
static bool lcm_set_recovery_notify(void)
{
	char tp_info[] = "nvt";

	//return TRUE if TP need lcm notify
	//NVT touch recover need enable lcm notify
	if (strstr(tp_info, "nvt")) {
		pr_info("%s: return TRUE\n", __func__);
		return TRUE;
	}

	return false;
}
#endif

struct LCM_DRIVER mipi_mot_vid_csot_nt36672c_fhd_678_lcm_drv = {
	.name = "mipi_mot_vid_csot_nt36672c_fhd_678",
	.supplier = "csot",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.suspend_power = lcm_suspend_power,
	.resume = lcm_resume,
#ifdef LCM_BL_DRV_I2C_SUPPORT
	.set_backlight = lcm_setbacklight,
#else
	.set_backlight_cmdq= lcm_setbacklight_cmdq,
#endif
	.ata_check = lcm_ata_check,
	.switch_mode = lcm_switch_mode,
#ifdef CONFIG_LCM_NOTIFIY_SUPPORT
	.set_lcm_notify = lcm_set_recovery_notify,
#endif
	.set_lcm_cmd = lcm_set_cmdq,
	.tp_gesture_status = GESTURE_OFF,
};
