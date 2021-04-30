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

#define BL_MAX_LEVEL                    1638

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
	{ 0xFF, 0x03, {0x78, 0x07, 0x00} },
	{0x28, 0, {} },
	{REGFLAG_DELAY, 20, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 120, {} }

};

static struct LCM_setting_table init_setting_vdo[] = {
	{0xFF,3,{0x78,0x07,0x06}}, //Page6
	{0xCD,1,{0x68}},
	{0xFF,3,{0x78,0x07,0x00}}, //Page0
	{0x53,1,{0x2C}},
	{0x51,2,{0x00,0x00}},
	{0x55,1,{0x01}}, 	//0x01 ui 	0x03 mv 	0x00 off
	{0x11,1,{0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0xFF,3,{0x78,0x07,0x03}}, //Page3, pwm clk to 18.8Khz
	{0x84,1,{0x02}},
	{0xFF,3,{0x78,0x07,0x00}}, //Page0, pwm clk to 18.8Khz
	{0x29,1,{0x00}},
	{REGFLAG_DELAY, 20, {}},
	{0x35,1,{0x00}}
};

static struct LCM_setting_table bl_level[] = {
	{ 0xFF, 0x03, {0x78, 0x07, 0x00} },
	{ 0x51, 0x02, {0x06, 0x66} },
	{ REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table lcm_cabc_setting_ui[] = {
	{0xFF, 3, {0x78, 0x07, 0x00} },
	{0x55, 1, {0x01} },
};

static struct LCM_setting_table lcm_cabc_setting_mv[] = {
	{0xFF, 3, {0x78, 0x07, 0x00} },
	{0x55, 1, {0x03} },
};

static struct LCM_setting_table lcm_cabc_setting_disable[] = {
	{0xFF, 3, {0x78, 0x07, 0x00} },
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

static struct LCM_setting_table lcm_hbm_setting[] = {
	{0x51, 2, {0x06, 0X66} },	//80% PWM
	{0x51, 2, {0x0F, 0XFF} },	//100% PWM
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
	params->dsi.horizontal_frontporch = 138;
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
	LCM_LOGI("[LCM] lcm_init_power\n");
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_PWR_EN_OUT1);
	MDELAY(5);
	lcm_set_bias_init(18,3);
	MDELAY(10);
}

static void lcm_suspend_power(void)
{
	LCM_LOGI("[LCM] lcm_suspend_power\n");
	lcm_set_bias_pin_disable(3);
	MDELAY(5);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_PWR_EN_OUT0);
	LCM_LOGI("[LCM] lcm suspend power down.\n");
}

static void lcm_resume_power(void)
{
	LCM_LOGI("[LCM] lcm_resume_power\n");
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_PWR_EN_OUT1);
	MDELAY(5);
	lcm_set_bias_pin_enable(18,3);
	MDELAY(10);
}

static void lcm_init(void)
{
	LCM_LOGI("[LCM] lcm_init\n");
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
	MDELAY(10);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT0);
	MDELAY(1);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
	MDELAY(20);
	push_table(NULL, init_setting_vdo, sizeof(init_setting_vdo) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	LCM_LOGI("[LCM] lcm_suspend\n");

	push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
	MDELAY(10);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
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
	MDELAY(10);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT0);
	MDELAY(1);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
	MDELAY(20);

	array[0] = 0x00043700;	// read id return four byte
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xA1, buffer, 4);

	LCM_LOGI("%s,ili7806s_id=0x%02x%02x%02x%02x\n", __func__,buffer[0], buffer[1],buffer[2],buffer[3]);

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

static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{
	// set 11bit
	unsigned int bl_lvl;

	if (level > BL_MAX_LEVEL) {
		LCM_LOGI("%s: ili7806s: level%d: exceed max bl:%d\n", __func__, level, BL_MAX_LEVEL);
	}
	bl_lvl = level;
	//for 11bit
	bl_level[1].para_list[0] = (bl_lvl&0x700)>>8;
	bl_level[1].para_list[1] = (bl_lvl&0xFF);
	LCM_LOGI("%s,ili7806s_txd : para_list[0]=0x%x,para_list[1]=0x%x\n",__func__,bl_level[1].para_list[0],bl_level[1].para_list[1]);

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
	pr_info("%s,ili7806s cmd:%d, value = %d\n", __func__, *lcm_cmd, *lcm_value);

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
			pr_err("%s,ili7807s cmd:%d, unsupport\n", __func__, *lcm_cmd);
			break;
	}

	pr_info("%s,ili7806s cmd:%d, value = %d done\n", __func__, *lcm_cmd, *lcm_value);

}

struct LCM_DRIVER mipi_mot_vid_txd_ili7806s_hdp_652_lcm_drv = {
	.name = "mipi_mot_vid_txd_ili7806s_hdp_652",
	.supplier = "txd",
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
	.set_lcm_cmd = lcm_set_cmdq,
#if (LCM_DSI_CMD_MODE)
	.validate_roi = lcm_validate_roi,
#endif

};
