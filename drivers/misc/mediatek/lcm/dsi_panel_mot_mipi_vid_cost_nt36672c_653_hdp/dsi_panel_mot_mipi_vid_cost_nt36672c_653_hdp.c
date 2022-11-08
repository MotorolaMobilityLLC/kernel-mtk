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
#include "lcm_util.h"

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

#define LCM_LOGI(fmt, args...)  pr_info("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)

#define LCM_ID_0			0x08
#define LCM_ID_1 			0x01
#define LCM_ID_2	 		0x65
#define LCM_ID_3 			0xA2

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

#define LCM_DSI_CMD_MODE	0
#define FRAME_WIDTH			(720)
#define FRAME_HEIGHT		(1600)

#define LCM_PHYSICAL_WIDTH	(68040)
#define LCM_PHYSICAL_HEIGHT	(151200)

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
	{REGFLAG_DELAY, 20, {}},
	{0x10, 0, {} },
	{REGFLAG_DELAY, 120, {}},
};

static struct LCM_setting_table init_setting_vdo[] = {
	{0XFF, 0x01, {0XE0}},
	{0XFB, 0x01, {0X01}},
	{0X35, 0x01, {0X82}},

	{0XFF, 0x01, {0XF0}},
	{0XFB, 0x01, {0X01}},
	{0X1C, 0x01, {0X01}},
	{0X33, 0x01, {0X01}},
	{0X5A, 0x01, {0X00}},

	{0XFF, 0x01, {0XD0}},
	{0XFB, 0x01, {0X01}},
	{0X53, 0x01, {0X22}},

	{0X54, 0x01, {0X02}},
	{0XFF, 0x01, {0XC0}},
	{0XFB, 0x01, {0X01}},
	{0X9C, 0x01, {0X11}},
	{0X9D, 0x01, {0X11}},

	{0XFF, 0x01, {0X2B}},
	{0XFB, 0x01, {0X01}},
	{0XB7, 0x01, {0X2A}},
	{0XB8, 0x01, {0X0D}},
	{0XC0, 0x01, {0X01}},

	{0xFF, 0x01, {0xF0}},
	{0xFB, 0x01, {0x01}},
	{0xD2, 0x01, {0x50}},

	{0XFF, 0x01, {0X10}},
	{0XFB, 0x01, {0X01}},

	{0X35, 0x01, {0X00}},
	{0x53, 0x01, {0x2C}},
	{0x55, 0x01, {0x01}},
	{0x68, 0x02, {0x00,0x01}},

	{0XB0, 0x01, {0X00}},
	{0XC0, 0x01, {0X00}},
	{0XC2, 0x02, {0X1B,0XA0}},

	{0x11, 0x01, {0x00}},
	{REGFLAG_DELAY,120,{}},
	{0x29, 0x01, {0x00}},
	{REGFLAG_DELAY, 20, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}},

};


static struct LCM_setting_table bl_level[] = {
	{0XFF, 0x01, {0X10}},
	{0XFB, 0x01, {0X01}},
	{0x51, 0x02, {0x3F, 0xFF}},
	{ REGFLAG_END_OF_TABLE, 0x00, {} },
};

static void push_table(void *cmdq, struct LCM_setting_table *table,
	unsigned int count, unsigned char force_update)
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
			dsi_set_cmdq_V22(cmdq, cmd, table[i].count,
				table[i].para_list, force_update);
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
	LCM_LOGI("[LCM] lcm_dfps_int start\n");
	dsi->dfps_enable = 1;
	dsi->dfps_default_fps = 9000;/*real fps * 100, to support float*/
	dsi->dfps_def_vact_tim_fps = 9000;/*real vact timing fps * 100*/

	/*traversing array must less than DFPS_LEVELS*/
	/*DPFS_LEVEL0*/
	dfps_params[0].level = DFPS_LEVEL0;
	dfps_params[0].fps = 6000;/*real fps * 100, to support float*/
	dfps_params[0].vact_timing_fps = 9000;/*real vact timing fps * 100*/
	/*if mipi clock solution*/
	/*dfps_params[0].PLL_CLOCK = xx;*/
	/*dfps_params[0].data_rate = xx; */
	/*if HFP solution*/
	/*dfps_params[0].horizontal_frontporch = xx;*/
	dfps_params[0].vertical_frontporch = 880;
//	dfps_params[0].vertical_frontporch_for_low_power = 980;

	/*if need mipi hopping params add here*/
	//dfps_params[0].dynamic_switch_mipi = 0;
	//dfps_params[0].PLL_CLOCK_dyn = 550;
	//dfps_params[0].horizontal_frontporch_dyn = 288;
	//dfps_params[0].vertical_frontporch_dyn = 1291;
	//dfps_params[0].vertical_frontporch_for_low_power_dyn = 2500;

	/*DPFS_LEVEL1*/
	dfps_params[1].level = DFPS_LEVEL1;
	dfps_params[1].fps = 9000;/*real fps * 100, to support float*/
	dfps_params[1].vact_timing_fps = 9000;/*real vact timing fps * 100*/
	/*if mipi clock solution*/
	/*dfps_params[1].PLL_CLOCK = xx;*/
	/*dfps_params[1].data_rate = xx; */
	/*if HFP solution*/
	/*dfps_params[1].horizontal_frontporch = xx;*/
	dfps_params[1].vertical_frontporch = 54;
	//dfps_params[1].vertical_frontporch_for_low_power = 980;//60 FPS in idle mode

	/*if need mipi hopping params add here*/
	//dfps_params[1].dynamic_switch_mipi = 0;
	//dfps_params[1].PLL_CLOCK_dyn = 550;
	//dfps_params[1].horizontal_frontporch_dyn = 288;
	//dfps_params[1].vertical_frontporch_dyn = 8;
	//dfps_params[1].vertical_frontporch_for_low_power_dyn = 2500;

	dsi->dfps_num = 2;
		LCM_LOGI("[LCM] lcm_dfps_int endn");
}
#endif

static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->physical_width = LCM_PHYSICAL_WIDTH/1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT/1000;
	params->physical_width_um = LCM_PHYSICAL_WIDTH;
	params->physical_height_um = LCM_PHYSICAL_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
	lcm_dsi_mode = CMD_MODE;
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
	lcm_dsi_mode = SYNC_PULSE_VDO_MODE;
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
	params->dsi.vertical_backporch = 18;
	params->dsi.vertical_frontporch = 40;
	//params->dsi.vertical_frontporch_for_low_power = 540;/*disable dynamic frame rate*/
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 4;
	params->dsi.horizontal_backporch = 26;
	params->dsi.horizontal_frontporch = 26;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_disable = 1;

#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 376;	/* this value must be in MTK suggested table */
#else
	params->dsi.PLL_CLOCK = 376;	/* this value must be in MTK suggested table */
#endif

	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;

#ifdef CONFIG_MTK_ROUND_CORNER_SUPPORT
	params->corner_pattern_width = 32;
	params->corner_pattern_height = 32;
#endif
#ifdef CONFIG_MTK_HIGH_FRAME_RATE
	/****DynFPS start****/
	lcm_dfps_int(&(params->dsi));
	/****DynFPS end****/
#endif
}

static void lcm_init_power(void)
{
	LCM_LOGI("[LCM] lcm_init_power\n");
	lcm_set_bias_init(15,3);
	MDELAY(10);
}

static void lcm_suspend_power(void)
{
	LCM_LOGI("[LCM] lcm_suspend_power\n");
	lcm_set_bias_pin_disable(3);
	MDELAY(10);
}

static void lcm_resume_power(void)
{
	LCM_LOGI("[LCM] lcm_resume_power\n");
	lcm_set_bias_pin_enable(15,3);
	MDELAY(10);
}

static void lcm_init(void)
{
	LCM_LOGI("[LCM] lcm_init\n");

	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(10);

	SET_RESET_PIN(1);
	MDELAY(10);

	push_table(NULL, init_setting_vdo,
		sizeof(init_setting_vdo) / sizeof(struct LCM_setting_table),
		1);
	pr_debug("tm_ili7807s----lcm mode = vdo mode :%d----\n",
		lcm_dsi_mode);

}

static void lcm_suspend(void)
{
	LCM_LOGD("[LCM] lcm_suspend\n");
	push_table(NULL, lcm_suspend_setting,
		sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table),
		1);
	MDELAY(10);

}

static void lcm_resume(void)
{
	/*pr_debug("lcm_resume\n");*/

	lcm_init();
}

static void lcm_update(unsigned int x, unsigned int y,
	unsigned int width, unsigned int height)
{
}

static unsigned int lcm_compare_id(void)
{
	unsigned char buffer[4];
	unsigned int array[16];

	LCM_LOGI("[LCM] lcm_compare_id enter\n");

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);

	SET_RESET_PIN(1);
	MDELAY(20);

	array[0] = 0x00043700;	// read id return four byte
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xA1, buffer, 4);

	LCM_LOGI("%s,cost_nov id=0x%02x%02x%02x%02x\n", __func__,buffer[0], buffer[1],buffer[2],buffer[3]);
	if (LCM_ID_0 == buffer[0] && LCM_ID_1 == buffer[1] && LCM_ID_2 == buffer[2] && LCM_ID_3 == buffer[3]) {
		LCM_LOGI("%s: cost_nov 653 matched\n", __func__);
		return 1;
	}
	else {
		LCM_LOGI("%s: cost_nov 653 not matched\n", __func__);
		return 0;
	}
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

	LCM_LOGI("ATA check size = 0x%x,0x%x,0x%x,0x%x\n",
		x0_MSB, x0_LSB, x1_MSB, x1_LSB);
	data_array[0] = 0x0005390A;	/* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00043700;/* read id return two byte,version and id */
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
	unsigned int bl_lvl = level;

	LCM_LOGI("%s,cost_nov backlight: level = %d\n,get default level = %d\n", __func__, level, bl_level[1].para_list[0]);

	//for 11bit
	bl_level[2].para_list[0] = (bl_lvl&0x700)>>8;
	bl_level[2].para_list[1] = (bl_lvl&0xFF);

	LCM_LOGI("%s:cost_nov: para_list[0]=0x%x,para_list[1]=0x%x\n",__func__,bl_level[1].para_list[0],bl_level[1].para_list[1]);
	push_table(handle, bl_level,
		sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}

struct LCM_DRIVER mipi_mot_vid_cost_nt36672c_hdp_653_lcm_drv = {
	.name = "mipi_mot_vid_cost_nt36672c_hdp_653",
	.supplier = "cost",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id     = lcm_compare_id,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.esd_check = lcm_esd_check,
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
	.ata_check = lcm_ata_check,
	.update = lcm_update,
};
