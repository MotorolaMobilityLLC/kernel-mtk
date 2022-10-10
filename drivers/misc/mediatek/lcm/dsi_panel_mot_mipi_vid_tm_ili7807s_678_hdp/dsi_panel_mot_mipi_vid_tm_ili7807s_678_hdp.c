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

#define LCM_ID_0			0x01
#define LCM_ID_1 			0x05
#define LCM_ID_2	 		0x07
#define LCM_ID_3 			0x91

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
#define FRAME_WIDTH			(1080)
#define FRAME_HEIGHT		(2460)

#define LCM_PHYSICAL_WIDTH	(69174)
#define LCM_PHYSICAL_HEIGHT	(157563)

#define CFG_BL_BITS_12			0
#define CFG_BL_BITS_11			1
#if CFG_BL_BITS_12
#define LCM_BL_MAX_BRIGHTENSS		4095
#elif CFG_BL_BITS_11
#define LCM_BL_MAX_BRIGHTENSS		2047
#else
#define LCM_BL_MAX_BRIGHTENSS		1023
#endif

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
	{REGFLAG_DELAY, 120, {} },
};

static struct LCM_setting_table init_setting_vdo[] = {
	{0xFF, 0x03, {0X78, 0X07, 0X06}},
	{0x3E, 0x01, {0xE2}},

	{0xFF, 0x03, {0X78, 0X07, 0X02}},
	{0x1B, 0x01, {0x02}},
	{0x06, 0x01, {0x69}},
	{0x08, 0x01, {0x00}},
	{0x0F, 0x01, {0x28}},

	{0xFF, 0x03, {0X78, 0X07, 0X07}},
	{0x29, 0x01, {0x80}},

	{0xFF, 0x03, {0X78, 0X07, 0X08}},
	{0xFD, 0x02, {0x00, 0x9F}},
	{0xE1, 0x01, {0xD9}},
	{0xFD, 0x02, {0x00, 0x00}},

	{0xFF, 0x03, {0X78, 0X07, 0X0B}},
	{0x95, 0x01, {0x14}},
	{0x9A, 0x01, {0x46}},
	{0x9B, 0x01, {0x91}},
	{0x9E, 0x01, {0xA4}},
	{0x9F, 0x01, {0xA4}},
	{0xAA, 0x01, {0x12}},

	{0xFF, 0x03, {0X78, 0X07, 0X0E}},
	{0x05, 0x01, {0x24}},
	{0x07, 0x01, {0x20}},
	{0x13, 0x01, {0x04}},
	{0x41, 0x01, {0x14}},
	{0x42, 0x01, {0x02}},
	{0x43, 0x01, {0x14}},
	{0x44, 0x01, {0x82}},
	{0x45, 0x01, {0x0B}},
	{0x46, 0x01, {0x36}},
	{0x47, 0x01, {0x10}},
	{0x49, 0x01, {0x34}},

	{0xB0, 0x01, {0x02}},
	{0xB1, 0x01, {0x5F}},
	{0xC0, 0x01, {0x12}},
	{0xC2, 0x01, {0x5F}},
	{0xC3, 0x01, {0x5F}},
	{0xC4, 0x01, {0x5F}},
	{0xC5, 0x01, {0x5F}},
	{0xC6, 0x01, {0x5F}},
	{0xC7, 0x01, {0x5F}},
	{0xC8, 0x01, {0x5F}},
	{0xC9, 0x01, {0x5F}},
	{0x4D, 0x01, {0xBC}},
	{0x50, 0x01, {0x00}},
	{0x4B, 0x01, {0x02}},

	{0xFF, 0x03, {0X78, 0X07, 0X1E}},
	{0xC9, 0x01, {0x02}},
	{0xC0, 0x01, {0x1F}},
	{0xC1, 0x01, {0x00}},
	{0xC2, 0x01, {0x00}},
	{0xC3, 0x01, {0x00}},
	{0xC4, 0x01, {0x1B}},
	{0xC7, 0x01, {0x00}},

	{0xFF, 0x03, {0X78, 0X07, 0X00}},
	{0x35, 0x01, {0x00}},
	{0x11, 0x01, {0x00}},
	{REGFLAG_DELAY,120,{}},
	{0x51, 0x02, {0x3F, 0xFF} },
	{0x53, 0x01, {0x2C}},
	{0x55, 0x01, {0x00}},
	{0x29, 0x01, {0x00}},
	{REGFLAG_DELAY, 20, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}},

};


static struct LCM_setting_table bl_level[] = {
	{ 0xFF, 0x03, {0x98, 0x81, 0x00} },
	{ 0x51, 0x02, {0x3F, 0xFF} },
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
	params->dsi.vertical_backporch = 12;
	params->dsi.vertical_frontporch = 46;
	//params->dsi.vertical_frontporch_for_low_power = 540;/*disable dynamic frame rate*/
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 4;
	params->dsi.horizontal_backporch = 26;
	params->dsi.horizontal_frontporch = 26;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_disable = 1;

#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 550;	/* this value must be in MTK suggested table */
#else
	params->dsi.PLL_CLOCK = 550;	/* this value must be in MTK suggested table */
#endif

	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
/*
	params->dsi.dsc_enable = 1;
	params->dsi.dsc_params.ver = 17;
	params->dsi.dsc_params.slice_mode = 1;
	params->dsi.dsc_params.rgb_swap = 0;
	params->dsi.dsc_params.dsc_cfg = 34;
	params->dsi.dsc_params.rct_on = 1;
	params->dsi.dsc_params.bit_per_channel = 8;
	params->dsi.dsc_params.dsc_line_buf_depth = 9;
	params->dsi.dsc_params.bp_enable = 1;
	params->dsi.dsc_params.bit_per_pixel = 128;  //128
	params->dsi.dsc_params.pic_height = 2408;
	params->dsi.dsc_params.pic_width = 1080;
	params->dsi.dsc_params.slice_height = 8;
	params->dsi.dsc_params.slice_width = 540;
	params->dsi.dsc_params.chunk_size = 540;
	params->dsi.dsc_params.xmit_delay = 170;
	params->dsi.dsc_params.dec_delay = 526;
	params->dsi.dsc_params.scale_value = 32;
	params->dsi.dsc_params.increment_interval = 43;
	params->dsi.dsc_params.decrement_interval = 7;
	params->dsi.dsc_params.line_bpg_offset = 12;
	params->dsi.dsc_params.nfl_bpg_offset = 3511;
	params->dsi.dsc_params.slice_bpg_offset = 3255;
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
*/
#ifdef CONFIG_MTK_ROUND_CORNER_SUPPORT
	params->corner_pattern_width = 32;
	params->corner_pattern_height = 32;
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

	LCM_LOGI("%s,tm_ili7807s id=0x%02x%02x%02x%02x\n", __func__,buffer[0], buffer[1],buffer[2],buffer[3]);

	if (LCM_ID_0 == buffer[0] && LCM_ID_1 == buffer[1] && LCM_ID_2 == buffer[2] && LCM_ID_3 == buffer[3]) {
		LCM_LOGI("lcm_compare_id:krn: tm_ili7807s 678 matched\n");
		return 1;
	}
	else {
		LCM_LOGI("lcm_compare_id:krn: tm_ili7807s 678 not matched!\n");
		return 0;
	}
}

/* return TRUE: need recovery */
/* return FALSE: No need recovery */
/*
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
*/

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

	//to 10bit
	bl_lvl = level*1023/LCM_BL_MAX_BRIGHTENSS;
	LCM_LOGI("%s:tm: 12 bit level = 0x%3x, 10 bit bl_lvl = 0x%3x\n", __func__, level, bl_lvl);
	if (!bl_lvl && level) {
		//minmum check to avoid black issue when trans
		bl_lvl = 1;
		LCM_LOGI("%s, reset 10bit bl_lvl=%d\n", __func__, bl_lvl);
	}

	//for 10bit
	bl_level[1].para_list[0] = (bl_lvl&0x300)>>8;
	bl_level[1].para_list[1] = (bl_lvl&0xFF);

	LCM_LOGI("%s:tm: para_list[0]=0x%x,para_list[1]=0x%x\n",__func__,bl_level[1].para_list[0],bl_level[1].para_list[1]);
	push_table(handle, bl_level,
		sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}

struct LCM_DRIVER mipi_mot_vid_tm_ili7807s_hdp_678_lcm_drv = {
	.name = "mipi_mot_vid_tm_ili7807s_hdp_678",
	.supplier = "tm",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id     = lcm_compare_id,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	//.esd_check = lcm_esd_check,
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
	.ata_check = lcm_ata_check,
	.update = lcm_update,
};
