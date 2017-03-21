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
#else
#include <linux/regulator/mediatek/mtk_regulator.h>
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
#define LCM_DSI_CMD_MODE									0
#define FRAME_WIDTH										(720)
#define FRAME_HEIGHT									(1280)

/* physical size in um */
#define LCM_PHYSICAL_WIDTH									(74520)
#define LCM_PHYSICAL_HEIGHT									(132480)

#define REGFLAG_DELAY		0xFFFC
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
	{REGFLAG_DELAY, 20, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 120, {} },
	{0x4F, 1, {0x01} },
	{REGFLAG_DELAY, 120, {} }
};

static struct LCM_setting_table lcm_initialization_setting[] = {
{0x00,1,{0x00}},
{0xFF,3,{0x12,0x89,0x01}},
{0x00,1,{0x80}},
{0xFF,2,{0x12,0x89}},
{0x00,1,{0x90}},
{0xFF,1,{0xB0}},

{0x00,1,{0x80}},
{0xC0,8,{0x4A,0x00,0x10,0x10,0x96,0x01,0x68,0x40}},

{0x00,1,{0x90}},
{0xC0,3,{0x3B,0x01,0x09}},

{0x00,1,{0x8C}},
{0xC0,1,{0x00}},

{0x00,1,{0x80}},
{0xC1,1,{0x33}},
//-------------------- power setting --------------------------------//
{0x00,1,{0x8B}},
{0xC0,1,{0x04}},

{0x00,1,{0x85}},
{0xC5,3,{0x0A,0x0A,0x69}},

{0x00,1,{0x00}},
{0xD8,2,{0x27,0x27}},

//{0x00,1,{0x00}},
//{0xD9,2,{0x00,0x58}},

{0x00,1,{0x84}},
{0xC4,1,{0x02}},

{0x00,1,{0x93}},
{0xC4,1,{0x04}},

{0x00,1,{0x96}},
{0xF5,1,{0xE7}},

{0x00,1,{0xA0}},
{0xF5,1,{0x4A}},

{0x00,1,{0x8A}},
{0xC0,1,{0x11}},

{0x00,1,{0x83}},
{0xF5,1,{0x81}},
//-------------------- for Power IC ----------------------------------//
{0x00,1,{0x90}},
{0xC4,2,{0x96,0x05}},
//-------------------- panel timing state control --------------------//
{0x00,1,{0x80}},
{0xCB,15,{0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x00,0x14,0x14,0x00,0x00,0x14}},

{0x00,1,{0x90}},
{0xCB,7,{0x14,0x00,0x00,0x00,0x00,0x00,0x00}},
//-------------------- panel pad mapping control ---------------------//
{0x00,1,{0x80}},
{0xCC,14,{0x06,0x1D,0x1E,0x1D,0x1D,0x10,0x0C,0x0E,0x0A,0x00,0x02,0x00,0x00,0x00}},

{0x00,1,{0x90}},
{0xCC,15,{0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x1D,0x1E,0x1D,0x1D,0x0F,0x0B,0x0D}},

{0x00,1,{0xA0}},
{0xCC,13,{0x09,0x00,0x10,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00}},

{0x00,1,{0xB0}},
{0xCC,14,{0x01,0x1E,0x1D,0x1D,0x1D,0x09,0x0D,0x0B,0x0F,0x00,0x05,0x00,0x00,0x00}},

{0x00,1,{0xC0}},
{0xCC,15,{0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x1e,0x1d,0x1d,0x1d,0x0a,0x0e,0x0c}},

{0x00,1,{0xD0}},
{0xCC,13,{0x10,0x00,0x06,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00}},
//-------------------- panel timing setting --------------------------//
{0x00,1,{0x80}},
{0xCE,6,{0x87,0x05,0x10,0x86,0x00,0x00}},

{0x00,1,{0x90}},
{0xCE,9,{0x34,0xff,0x10,0x05,0x00,0x00,0x00,0x00,0x00}},

{0x00,1,{0xA0}},
{0xCE,15,{0x30,0x83,0x86,0x87,0x16,0x00,0x82,0x85,0x87,0x81,0x84,0x87,0x80,0x83,0x87}},

{0x00,1,{0xB0}},
{0xCE,15,{0x30,0x00,0x82,0x87,0x16,0x00,0x01,0x81,0x87,0x02,0x80,0x87,0x03,0x00,0x87}},

{0x00,1,{0xF0}},
{0xCE,6,{0x01,0x20,0xC1,0x81,0x00,0x00}},
//-------------------- gamma -----------------------------------------//
{0x00,1,{0x00}},
{0xE1,16,{0x00,0x16,0x26,0x37,0x47,0x65,0x64,0x7d,0x76,0x62,0x69,0x50,0x38,0x21,0x13,0x00}},

{0x00,1,{0x00}},
{0xE2,16,{0x00,0x16,0x26,0x37,0x47,0x65,0x64,0x7d,0x76,0x62,0x69,0x50,0x38,0x21,0x13,0x00}},

{0x00,1,{0x00}},
{0xFF,3,{0xFF,0xFF,0xFF}},

{0x11,1,{0x00}},  
{REGFLAG_DELAY, 120, {}},            
{0x29,1,{0x00}},       
{REGFLAG_DELAY, 10, {}}, 

{0x51,1,{0xff}},
{REGFLAG_DELAY, 5, {}},
{0x53,1,{0x24}},
{REGFLAG_DELAY, 5, {}},
{0x55,1,{0x01}},
{REGFLAG_DELAY, 5, {}},

{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table bl_level[] = {
	{0x51,1,{0xff}},
	{REGFLAG_END_OF_TABLE, 0x00, {} }
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

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
	lcm_dsi_mode = CMD_MODE;
#else
	params->dsi.mode = BURST_VDO_MODE;
#endif

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

	params->dsi.vertical_sync_active				= 4;//15
	params->dsi.vertical_backporch					= 14;//20
	params->dsi.vertical_frontporch					= 12; //20
	params->dsi.vertical_active_line				= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active				= 20; //20
	params->dsi.horizontal_backporch				= 50;  //140
	params->dsi.horizontal_frontporch				= 50;  //100
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;
	params->dsi.ssc_disable = 1;

/*add by lct jiatianbao start 20170321*/
#ifdef CONFIG_LCT_DEVINFO_SUPPORT                                                                                                                                                                       
      params->vendor="Wistron";
      params->ic="otm1289a";
      params->info="720*1280";
#endif
/*add by lct jiatianbao end 20170321*/

#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 150;	/* this value must be in MTK suggested table */
#else
	params->dsi.PLL_CLOCK = 207;	/* this value must be in MTK suggested table */
#endif
	params->dsi.cont_clock=0;
	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;

}

static struct mtk_regulator disp_bias_pos;
static struct mtk_regulator disp_bias_neg;
static int regulator_inited;

static int display_bias_regulator_init(void)
{
	int ret;

	if (!regulator_inited) {
		/* please only get regulator once in a driver */
		ret = mtk_regulator_get(NULL, "dsv_pos", &disp_bias_pos);
		if (ret < 0) { /* handle return value */
			pr_err("get dsv_pos fail\n");
			return -1;
		}

		ret = mtk_regulator_get(NULL, "dsv_neg", &disp_bias_neg);
		if (ret < 0) { /* handle return value */
			pr_err("get dsv_pos fail\n");
			return -1;
		}

		regulator_inited = 1;
		return ret;
	}
	return 0;
}

static int display_bias_enable(void)
{
	int ret = 0;

	ret = display_bias_regulator_init();
	if (ret)
		return ret;

	/* set voltage with min & max*/
	ret = mtk_regulator_set_voltage(&disp_bias_pos, 5400000, 5400000);
	if (ret < 0)
		pr_err("set voltage disp_bias_pos fail\n");

	ret = mtk_regulator_set_voltage(&disp_bias_neg, 5400000, 5400000);
	if (ret < 0)
		pr_err("set voltage disp_bias_neg fail\n");

#if 0
	/* get voltage */
	ret = mtk_regulator_get_voltage(&disp_bias_pos);
	if (ret < 0)
		pr_err("get voltage disp_bias_pos fail\n");
	pr_debug("pos voltage = %d\n", ret);

	ret = mtk_regulator_get_voltage(&disp_bias_neg);
	if (ret < 0)
		pr_err("get voltage disp_bias_neg fail\n");
	pr_debug("neg voltage = %d\n", ret);
#endif
	/* enable regulator */
	ret = mtk_regulator_enable(&disp_bias_pos, true);
	if (ret < 0)
		pr_err("enable regulator disp_bias_pos fail\n");

	ret = mtk_regulator_enable(&disp_bias_neg, true);
	if (ret < 0)
		pr_err("enable regulator disp_bias_neg fail\n");
	return ret;
}

static int display_bias_disable(void)
{
	int ret = 0;

	ret = display_bias_regulator_init();
	if (ret)
		return ret;

	ret |= mtk_regulator_enable(&disp_bias_neg, false);
	if (ret < 0)
		pr_err("disable regulator disp_bias_neg fail\n");

	ret |= mtk_regulator_enable(&disp_bias_pos, false);
	if (ret < 0)
		pr_err("disable regulator disp_bias_pos fail\n");

	return ret;
}

static void lcm_init_power(void)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
	/* display bias is likely inited in lk !!
	 * for kernel regulator system, we need to enable it first before disable!
	 * so here, if bias is not enabled, we enable it first
	 */
	display_bias_enable();
#endif
}

static void lcm_suspend_power(void)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
	display_bias_disable();
#endif
}

static void lcm_resume_power(void)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
	SET_RESET_PIN(0);
	display_bias_enable();
#endif

}

static void lcm_init(void)
{
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(5);
	SET_RESET_PIN(0);
	MDELAY(5);

	SET_RESET_PIN(1);
	MDELAY(100);
	push_table(NULL, lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
	push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);

    SET_RESET_PIN(0);
	MDELAY(10);
}

static void lcm_resume(void)
{
	lcm_init();
}

static unsigned int lcm_compare_id(void)
{
  	int   array[4];
	char  buffer[10];
	char  id0,id1,id2,id3,id4=0;
	
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(20);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(120);

	array[0] = 0x00053700;
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0xA1, buffer, 5);
	id0  =  buffer[0];
	id1  =  buffer[1];
	id2  =  buffer[2];
	id3  =  buffer[3];
	id4  =  buffer[4]; 
    printk("[Simon]LCM otm1289a %s: id0 = 0x%08x, id1 = 0x%08x, id2 = 0x%08x, id3 = 0x%08x\n", __func__, id0, id1, id2, id3);
 
	if ((id0 == 0x01)&&(id1 == 0x8B)&&(id2 == 0x12)&&(id3 == 0x89))
		return 1;
	else
        return 0;
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
	LCM_LOGI("%s,lcm otm1289a backlight: level = %d\n", __func__, level);

	bl_level[0].para_list[0] = level;

	push_table(handle, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}

LCM_DRIVER lct_otm1289a_wistron_720p_vdo_lcm_drv = {

	.name = "lct_otm1289a_wistron_720p_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id = lcm_compare_id,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
	.ata_check = lcm_ata_check,


};
