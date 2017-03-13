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

/* add by lct wangjiaxing for hbm 20170302 start */
#define LCT_LCM_MAPP_BACKLIGHT	  1
#ifdef CONFIG_LCT_CABC_MODE_SUPPORT
extern int cabc_mode_mode;
#define CABC_MODE_SETTING_UI	1
#define CABC_MODE_SETTING_MV	2
#define CABC_MODE_SETTING_DIS	3
#define CABC_MODE_SETTING_NULL	0
//static int reg_mode = 0;
#endif
/* add by lct wangjiaxing for hbm 20170302 end */

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

/* add by lct wangjiaxing for hbm 20170302 start */
static struct LCM_setting_table lcm_setting_ui[] = {
{0xFF,3,{0x98,0x81,0x00}},
{0x53,1,{0x24}},
{0x55,1,{0x01}},
{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_setting_dis[] = {
{0xFF,3,{0x98,0x81,0x00}},
{0x53,1,{0x24}},
{0x55,1,{0x00}},
{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_setting_mv[] = {
{0xFF,3,{0x98,0x81,0x00}},
{0x53,1,{0x2c}},
{0x55,1,{0x03}},
{REGFLAG_END_OF_TABLE, 0x00, {}}
};
/* add by lct wangjiaxing for hbm 20170302 end */

static struct LCM_setting_table lcm_initialization_setting[] = {

{0xFF,3,{0x98,0x81,0x03}},
//GIP_1
{0x01,1,{0x00}},
{0x02,1,{0x00}},
{0x03,1,{0x73}},
{0x04,1,{0x00}},
{0x05,1,{0x00}},
{0x06,1,{0x08}},
{0x07,1,{0x00}},
{0x08,1,{0x00}},
{0x09,1,{0x1B}},
{0x0a,1,{0x01}},
{0x0b,1,{0x01}},
{0x0c,1,{0x0D}},
{0x0d,1,{0x01}},
{0x0e,1,{0x01}},
{0x0f,1,{0x26}},
{0x10,1,{0x26}},
{0x11,1,{0x00}},
{0x12,1,{0x00}},
{0x13,1,{0x02}},
{0x14,1,{0x00}},
{0x15,1,{0x00}},
{0x16,1,{0x00}},
{0x17,1,{0x00}},
{0x18,1,{0x00}},
{0x19,1,{0x00}},
{0x1a,1,{0x00}},
{0x1b,1,{0x00}},
{0x1c,1,{0x00}},
{0x1d,1,{0x00}},
{0x1e,1,{0x40}},
{0x1f,1,{0xC0}},
{0x20,1,{0x06}},
{0x21,1,{0x01}},
{0x22,1,{0x06}},
{0x23,1,{0x01}},
{0x24,1,{0x88}},
{0x25,1,{0x88}},
{0x26,1,{0x00}},
{0x27,1,{0x00}},
{0x28,1,{0x3B}},
{0x29,1,{0x03}},
{0x2a,1,{0x00}},
{0x2b,1,{0x00}},
{0x2c,1,{0x00}},
{0x2d,1,{0x00}},
{0x2e,1,{0x00}},
{0x2f,1,{0x00}},
{0x30,1,{0x00}},
{0x31,1,{0x00}},
{0x32,1,{0x00}},
{0x33,1,{0x00}},
{0x34,1,{0x00}},
{0x35,1,{0x00}},
{0x36,1,{0x00}},
{0x37,1,{0x00}},
{0x38,1,{0x00}},
{0x39,1,{0x00}},
{0x3a,1,{0x00}},
{0x3b,1,{0x00}},
{0x3c,1,{0x00}},
{0x3d,1,{0x00}},
{0x3e,1,{0x00}},
{0x3f,1,{0x00}},
{0x40,1,{0x00}},
{0x41,1,{0x00}},
{0x42,1,{0x00}},
{0x43,1,{0x00}},
{0x44,1,{0x00}},
//GIP_2
{0x50,1,{0x01}},
{0x51,1,{0x23}},
{0x52,1,{0x45}},
{0x53,1,{0x67}},
{0x54,1,{0x89}},
{0x55,1,{0xab}},
{0x56,1,{0x01}},
{0x57,1,{0x23}},
{0x58,1,{0x45}},
{0x59,1,{0x67}},
{0x5a,1,{0x89}},
{0x5b,1,{0xab}},
{0x5c,1,{0xcd}},
{0x5d,1,{0xef}},
//GIP_3
{0x5e,1,{0x11}},
{0x5f,1,{0x02}},
{0x60,1,{0x00}},
{0x61,1,{0x07}},
{0x62,1,{0x06}},
{0x63,1,{0x0E}},
{0x64,1,{0x0F}},
{0x65,1,{0x0C}},
{0x66,1,{0x0D}},
{0x67,1,{0x02}},
{0x68,1,{0x02}},
{0x69,1,{0x02}},
{0x6a,1,{0x02}},
{0x6b,1,{0x02}},
{0x6c,1,{0x02}},
{0x6d,1,{0x02}},
{0x6e,1,{0x02}},
{0x6f,1,{0x02}},
{0x70,1,{0x02}},
{0x71,1,{0x02}},
{0x72,1,{0x02}},
{0x73,1,{0x05}},
{0x74,1,{0x01}},
{0x75,1,{0x02}},
{0x76,1,{0x00}},
{0x77,1,{0x07}},
{0x78,1,{0x06}},
{0x79,1,{0x0E}},
{0x7a,1,{0x0F}},
{0x7b,1,{0x0C}},
{0x7c,1,{0x0D}},
{0x7d,1,{0x02}},
{0x7e,1,{0x02}},
{0x7f,1,{0x02}},
{0x80,1,{0x02}},
{0x81,1,{0x02}},
{0x82,1,{0x02}},
{0x83,1,{0x02}},
{0x84,1,{0x02}},
{0x85,1,{0x02}},
{0x86,1,{0x02}},
{0x87,1,{0x02}},
{0x88,1,{0x02}},
{0x89,1,{0x05}},
{0x8A,1,{0x01}},
//CMD_Page 4
{0xFF,3,{ 0x98,0x81,0x04}},
//{0x00,1,{0x80}},           // 00-3LANE   80-4LANE 
{0x6C,1,{0x15}},                //Set VCORE voltage =1.5V);
{0x6E,1,{0x19}},              //di_pwr_reg=0 for power mode 
{0x6F,1,{0x25}},                // reg vcl + pumping ratio 
//{0x3A,1,{0x1F}},                //POWER SAVING);
{0x8D,1,{0x1F}},              //VGL clamp -10
{0x87,1,{0xBA}},               //ESD               );
{0x7A,1,{0x10}},
{0x71,1,{0xB0}},
//{0x26,1,{0x76}},            
//{0xB2,1,{0xD1}},
//{0x88,1,{0x0B}},
//CMD_Page 1
{0xFF,3,{ 0x98,0x81,0x01}},
{0x22,1,{0x0A}},               //BGR,0x SS);
{0x31,1,{0x00}},               //column inversion);
//{0x34,1,{0x01}},
{0x40,1,{0x53}},
{0x43,1,{0x66}},
{0x53,1,{0x7C}},
{0x55,1,{0x64}},               //VCOM1);  7c
//{0x56,1,{0x00}},       // FOR ²âÊÔÓÃ  ÓÃR53 ÉèÖÃµÄVCOMÖµ   
{0x50,1,{0x97}},               // VREG1OUT=4.7V);
{0x51,1,{0x92}},               // VREG2OUT=-4.7V);
{0x60,1,{0x15}},               //SDT);
{0x61,1,{0x01}}, 
{0x62,1,{0x0C}}, 
{0x63,1,{0x00}}, 

{0xA0,1,{0x00}},               //VP255 Gamma P);
{0xA1,1,{0x1A}},               //VP251);
{0xA2,1,{0x29}},               //VP247);
{0xA3,1,{0x14}},               //VP243);
{0xA4,1,{0x17}},               //VP239);
{0xA5,1,{0x29}},               //VP231);
{0xA6,1,{0x1D}},               //VP219);
{0xA7,1,{0x1E}},               //VP203);
{0xA8,1,{0x88}},               //VP175);
{0xA9,1,{0x1D}},               //VP144);
{0xAA,1,{0x29}},               //VP111);
{0xAB,1,{0x76}},              //VP80);
{0xAC,1,{0x19}},              //VP52);
{0xAD,1,{0x17}},              //VP36);
{0xAE,1,{0x4A}},              //VP24);
{0xAF,1,{0x20}},              //VP16);
{0xB0,1,{0x26}},               //VP12);
{0xB1,1,{0x4C}},               //VP8);
{0xB2,1,{0x5D}},              //VP4);
{0xB3,1,{0x3F}},              //VP0);
{0xC0,1,{0x00}},              //VN255 GAMMA N);
{0xC1,1,{0x1A}},              //VN251);
{0xC2,1,{0x29}},              //VN247);
{0xC3,1,{0x13}},              //VN243);
{0xC4,1,{0x16}},              //VN239);
{0xC5,1,{0x28}},              //VN231);
{0xC6,1,{0x1D}},              //VN219);
{0xC7,1,{0x1E}},              //VN203);
{0xC8,1,{0x88}},              //VN175); 
{0xC9,1,{0x1C}},               //VN144);
{0xCA,1,{0x29}},               //VN111);
{0xCB,1,{0x76}},               //VN80);
{0xCC,1,{0x19}},               //VN52);
{0xCD,1,{0x16}},               //VN36);
{0xCE,1,{0x4A}},               //VN24);
{0xCF,1,{0x1F}},               //VN16);
{0xD0,1,{0x26}},               //VN12);
{0xD1,1,{0x4B}},               //VN8);
{0xD2,1,{0x5D}},               //VN4);
{0xD3,1,{0x3F}},
{0xFF,3,{0x98,0x81,0x00}},
{0xFF,3,{0x98,0x81,0x02}},		//CMD_Page 0
{0x06,1,{0x40}},
{0x07,1,{0x05}}, //pwm  20K
{0xFF,3,{0x98,0x81,0x00}},
{0x68,2,{0x04,0x01}},
{0x11,1,{0x00}},  
{REGFLAG_DELAY, 120, {}},            
{0x29,1,{0x00}},       
{REGFLAG_DELAY, 10, {}},     
{0x35,1,{0x00}},   // te on
	{0x51,2,{0x0f,0xf0}},   
    {0x53,1,{0x24}},
     {0x55,1,{0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table bl_level[] = {
	{0x51, 2, {0x0F,0xF0} },
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

/*add by lct wangjiaxing start 20170313*/
#ifdef CONFIG_LCT_DEVINFO_SUPPORT                                                                                                                                                                       
     params->vendor="dijing";
     params->ic="ili9881c";
     params->info="720*1280";
#endif
/*add by lct wangjiaxing end 20170313*/
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

/* add by lct wangjiaxing for hbm 20170302 start */
#ifdef CONFIG_LCT_CABC_MODE_SUPPORT
static void push_table_v22(void *handle, struct LCM_setting_table *table, unsigned int count,
		       unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {

		unsigned cmd;
		void *cmdq = handle;
		cmd = table[i].cmd;

		switch (cmd) {

		case REGFLAG_DELAY:
			MDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V22(cmdq, cmd, table[i].count, table[i].para_list, force_update);
		}
	}

}

static void lcm_cabc_cmdq(void *handle, unsigned int mode)
{
    //printk("kls lcm_cabc_cmdq entry\n");
	switch(mode)
	{
		case CABC_MODE_SETTING_UI:
			{
				push_table_v22(handle,lcm_setting_ui,
			   sizeof(lcm_setting_ui) / sizeof(struct LCM_setting_table), 1);
			}
		break;
		case CABC_MODE_SETTING_MV:
			{
				push_table_v22(handle,lcm_setting_mv,
			   sizeof(lcm_setting_mv) / sizeof(struct LCM_setting_table), 1);
			}
		break;
		case CABC_MODE_SETTING_DIS:
			{
				push_table_v22(handle,lcm_setting_dis,
			   sizeof(lcm_setting_dis) / sizeof(struct LCM_setting_table), 1);
			}
		break;
		default:
		{
			push_table_v22(handle,lcm_setting_ui,
			   sizeof(lcm_setting_ui) / sizeof(struct LCM_setting_table), 1);
		}

	}
}
#endif

#ifdef CONFIG_LCT_HBM_SUPPORT
static unsigned int last_level=0;
static unsigned int hbm_enable=0;
static void lcm_setbacklight_hbm(unsigned int level)                                                                                                                                                    
{

	unsigned int level_hight,level_low=0;
printk("kls lcm_setbacklight_hbm entry level = %d\n",level);
	if(level==0)
	{
		level = last_level;
		hbm_enable = 0;
	}
	else
	  hbm_enable = 1;
	level_hight=(level & 0xf0)>>4;
	level_low=(level & 0x0f)<<4;
	bl_level[0].para_list[0] = level_hight;
	bl_level[0].para_list[1] = level_low;
	push_table(NULL, bl_level,
				sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}
/* add by lct wangjiaxing for hbm 20170302 end */

#endif
static void lcm_init(void)
{
	SET_RESET_PIN(0);
	MDELAY(5);
	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(10);

	SET_RESET_PIN(1);
	MDELAY(100);
	push_table(NULL, lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
	push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
	MDELAY(10);
	/* SET_RESET_PIN(0); */
}

static void lcm_resume(void)
{
	lcm_init();
}

static unsigned int lcm_compare_id(void)
{
  	int   array[4];
	char  buffer[3];
//	char  id0,id1,id=0;
   char id0 = 0;	
	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(10);

	SET_RESET_PIN(1);
	MDELAY(100);
	
	array[0] = 0x00043902; 						 
		array[1] = 0x008198ff; 				
	    dsi_set_cmdq(array, 2, 1); 
/*
		array[0] = 0x00013700;// read id return two byte,version and id
		dsi_set_cmdq(array, 1, 1);
		read_reg_v2(0x00, buffer, 1);
		id0  =  buffer[0];

		array[0] = 0x00013700;// read id return two byte,version and id
		dsi_set_cmdq(array, 1, 1);
		read_reg_v2(0x01, buffer, 1);
		id1  =  buffer[0]; 

	
		    id = (id0 << 8) | id1;
		printf("[Simon]ili9881-c %s, id0 = 0x%08x , id1 = 0x%08x, id = 0x%08x\n", __func__, id0, id1, id);

        if ((id0 == 0x98)&&(id1 == 0x81)) //cpt    
	   		return 1;
        else
           return 0;
*/

		array[0] = 0x00013700;// read id return two byte,version and id
		dsi_set_cmdq(array, 1, 1);
		read_reg_v2(0xDA, buffer, 1);
		id0  =  buffer[0]; 

	
		   
		printk("ili9881-c %s, id0 = 0x%08x \n", __func__, id0);

        if (id0 == 0x00)   
	   		
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

/* add by lct wangjiaxing for hbm 20170302 start */
static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{
	unsigned int level_hight,level_low=0;
#if(LCT_LCM_MAPP_BACKLIGHT)
	static unsigned int mapped_level = 0;
	mapped_level = (7835*level + 2165)/(10000);
#endif
	if(hbm_enable==0)
	{       

		printk("kls hbm_enable=0 level = %d\n",mapped_level);
		level_hight=(mapped_level & 0xf0)>>4;
		level_low=(mapped_level & 0x0f)<<4;
		bl_level[0].para_list[0] = level_hight;
		bl_level[0].para_list[1] = level_low;
		push_table(handle, bl_level,
					sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);

	}
	else
	{
		//printk("kls hbm_enable=1\n");
		level_hight=(255 & 0xf0)>>4;
		level_low=(255 & 0x0f)<<4;
		bl_level[0].para_list[0] = level_hight;
		bl_level[0].para_list[1] = level_low;
		push_table(handle, bl_level,
					sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
	}
#ifdef CONFIG_LCT_HBM_SUPPORT
	last_level = mapped_level;
#endif
}
/* add by lct wangjiaxing for hbm 20170302 end */

LCM_DRIVER lct_ili9881c_dijing_720p_vdo_lcm_drv = {

	.name = "lct_ili9881c_dijing_720p_vdo",
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
	/* add by lct wangjiaxing for hbm 20170302 start */
#ifdef CONFIG_LCT_CABC_MODE_SUPPORT
	.set_cabc_cmdq = lcm_cabc_cmdq,
#endif
#ifdef CONFIG_LCT_HBM_SUPPORT
     .set_backlight_hbm = lcm_setbacklight_hbm,
#endif
      /* add by lct wangjiaxing for hbm 20170302 end */
 

};
