// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
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
#include "../../gate_ic/lcd_bias.h"
#include "../../../../../drivers/leds/leds.h"

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
#define read_reg(cmd)	lcm_util.dsi_dcs_read_lcm_reg(cmd)
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
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#endif

#define FRAME_WIDTH			(720)
#define FRAME_HEIGHT			(1600)

/* physical size in um */
#define LCM_PHYSICAL_WIDTH		(64500)
#define LCM_PHYSICAL_HEIGHT		(129000)
#define LCM_DENSITY			(480)

#define REGFLAG_DELAY			0xFFFC
#define REGFLAG_UDELAY			0xFFFB
#define REGFLAG_END_OF_TABLE		0xFFFD
#define REGFLAG_RESET_LOW		0xFFFE
#define REGFLAG_RESET_HIGH		0xFFFF

#define LCM_ID_ILI9883A 0x01
#define BRINGHT_BIT_SWITCH	(8)
#define SHITF_EIGHT	(8)

#define BIAS_OUT_VALUE		(6000)

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define MAX_BRIGHTNESS 2047
#define MIN_BRIGHTNESS 19
#define NORMAL_MODE_RATIO 8
#define TEN 10
#define HBM_DIFF 4

//extern void get_hbm_status(void);

static bool sleepout_flag;

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
};

static struct LCM_setting_table init_setting_vdo[] = {
	{0xFF, 3, {0x78,0x07,0x06}},
	{0x1B, 1, {0x00}},
 	{0x3E, 1, {0xE2}},
	{0x12, 1, {0xBD}},
	{0xC6, 1, {0x40}},
	{0xFF, 3, {0x78,0x07,0x07}},
	{0x11, 1, {0x16}},
	{0x29, 1, {0x00}},
 	{0xFF, 3, {0x78,0x07,0x02}},
 	{0x02, 1, {0x00}},
 	{0x1B, 1, {0x00}},
 	{0xFF, 3, {0x78,0x07,0x00}},
 	{0x35, 1, {0x00}},
 	{0x11, 0, {}},
 	{REGFLAG_DELAY,120 , {}},
	{0xFF, 3, {0x78,0x07,0x00}},
 	{0x53, 1, {0x2c}},
 	{0x55, 1, {0x01}},
 	{0x29,0,{}},
 	{REGFLAG_DELAY, 20 , {}},
};

static struct LCM_setting_table
__maybe_unused lcm_deep_sleep_mode_in_setting[] = {
	{0x28, 1, {0x00} },
	{REGFLAG_DELAY, 50, {} },
	{0x10, 1, {0x00} },
	{REGFLAG_DELAY, 150, {} },
};

static struct LCM_setting_table __maybe_unused lcm_sleep_out_setting[] = {
	{0x11, 1, {0x00} },
	{REGFLAG_DELAY, 120, {} },
	{0x29, 1, {0x00} },
	{REGFLAG_DELAY, 50, {} },
};

static struct LCM_setting_table bl_level[] = {
	{0x51, 2, {0x07, 0xFF} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
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
			break;
		}
	}
}

static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}

#ifdef CONFIG_MTK_HIGH_FRAME_RATE
/*DynFPS*/
static void lcm_dfps_int(struct LCM_DSI_PARAMS *dsi)
{
	struct dfps_info *dfps_params = dsi->dfps_params;

	dsi->dfps_enable = 1;
	dsi->dfps_default_fps = 6000;/*real fps * 100, to support float*/
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

	dfps_params[0].vertical_frontporch = 870;
	dfps_params[0].vertical_frontporch_for_low_power = 0;
	/*if 60fps not decrease fps when idle*/
	/*dfps_params[0].vertical_frontporch_for_low_power = 0;*/

	/*if need mipi hopping params add here*/
	/*dfps_params[0].dynamic_switch_mipi = 1;
	dfps_params[0].PLL_CLOCK_dyn = 448;
	dfps_params[0].horizontal_backporch_dyn = 88;
	dfps_params[0].horizontal_frontporch_dyn = 88;*/

	//dfps_params[0].vertical_frontporch_for_low_power_dyn = 2500;*/
	/*if 60fps not decrease fps when idle*/
	/*dfps_params[0].vertical_frontporch_for_low_power_dyn = 0;*/

	/*DPFS_LEVEL1*/
	dfps_params[1].level = DFPS_LEVEL1;
	dfps_params[1].fps = 9000;/*real fps * 100, to support float*/
	dfps_params[1].vact_timing_fps = 9000;/*real vact timing fps * 100*/
	/*if mipi clock solution*/
	/*dfps_params[1].PLL_CLOCK = xx;*/
	/*dfps_params[1].data_rate = xx; */
	/*if HFP solution*/
	/*dfps_params[1].horizontal_frontporch = xx;*/
	dfps_params[1].vertical_frontporch = 40;
	dfps_params[1].vertical_frontporch_for_low_power = 0;
	/*if 90fps decrease to 60fps when idle*/
	/*dfps_params[1].vertical_frontporch_for_low_power = 1291;*/

	/*if need mipi hopping params add here*/
	/*dfps_params[1].dynamic_switch_mipi = 1;
	dfps_params[1].PLL_CLOCK_dyn = 448;
	dfps_params[0].horizontal_backporch_dyn = 88;
	dfps_params[1].horizontal_frontporch_dyn = 88;*/
	//dfps_params[1].vertical_frontporch_for_low_power_dyn = 2500;
	/*if 90fps decrease to 60fps when idle*/
	/*dfps_params[1].vertical_frontporch_for_low_power_dyn = 1291;*/

	dsi->dfps_num = 2;
}
#endif

static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->physical_width = LCM_PHYSICAL_WIDTH / 1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT / 1000;
	params->physical_width_um = LCM_PHYSICAL_WIDTH;
	params->physical_height_um = LCM_PHYSICAL_HEIGHT;
	params->density = LCM_DENSITY;

	lcm_dsi_mode = BURST_VDO_MODE;
	params->dsi.switch_mode = BURST_VDO_MODE;
	params->dsi.mode = BURST_VDO_MODE;

	LCM_LOGI("%s: lcm_dsi_mode %d\n", __func__, lcm_dsi_mode);
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
	params->dsi.vertical_frontporch = 870;
	params->dsi.vertical_frontporch_for_low_power = 0;	//OTM no data
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 20;
	params->dsi.horizontal_backporch = 102;
	params->dsi.horizontal_frontporch = 104;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_disable = 1;
#ifndef CONFIG_FPGA_EARLY_PORTING
	/* this value must be in MTK suggested table */
	params->dsi.PLL_CLOCK = 456;
	//params->dsi.PLL_CK_CMD = 419;
#else
	params->dsi.pll_div1 = 0;
	params->dsi.pll_div2 = 0;
	params->dsi.fbk_div = 0x1;
#endif
	params->dsi.CLK_HS_POST = 36;
	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0a;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;

	/* for ARR 2.0 */
	/*params->max_refresh_rate = 90;*/
	/*params->min_refresh_rate = 60;*/

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

static int lcm_bias_enable(void)
{
	lcd_bias_set_vspn(1, 0, BIAS_OUT_VALUE); /* Set the output voltage to 6v */
	pr_info("lcm ili7807s bias enable!\n");

	return 0;
}

static int lcm_bias_disable(void)
{
	lcd_bias_set_vspn(0, 1, BIAS_OUT_VALUE); /* Set the output voltage to 6v */
	pr_info("lcm ili7807s bias disable!\n");

	return 0;
}

/* turn on gate ic & control voltage to 6V */
static void lcm_init_power(void)
{
	lcm_bias_enable();
}

static void lcm_suspend_power(void)
{
	SET_RESET_PIN(0);
	MDELAY(3);
	lcm_bias_disable();
}

/* turn on gate ic & control voltage to 6V */
static void lcm_resume_power(void)
{
	lcm_init_power();
}

static void lcm_init(void)
{
	SET_RESET_PIN(1);
	MDELAY(2);
	SET_RESET_PIN(0);
	MDELAY(2);
	SET_RESET_PIN(1);
	MDELAY(7);
	push_table(NULL,
		init_setting_vdo, ARRAY_SIZE(init_setting_vdo), 1);
	sleepout_flag = TRUE;
	LCM_LOGI(
		"lcm ili7807s dsi mode=vdo mode:%d\n", lcm_dsi_mode);
}

static void lcm_suspend(void)
{
	MDELAY(2);
	push_table(NULL, lcm_suspend_setting,
		ARRAY_SIZE(lcm_suspend_setting), 1);
	set_hbm_status(0);
}

static void lcm_resume(void)
{
	lcm_init();
}

static unsigned int lcm_ata_check(unsigned char *buffer)
{
#ifndef BUILD_LK
	unsigned int ret = 0;
	unsigned int id[3] = {0x40, 0, 0};
	unsigned int data_array[3];
	unsigned char read_buf[3];

	data_array[0] = 0x00033700; /* set max return size = 3 */
	dsi_set_cmdq(data_array, 1, 1);

	read_reg_v2(0x04, read_buf, 3); /* read lcm id */

	LCM_LOGI("ATA read = 0x%x, 0x%x, 0x%x\n",
		 read_buf[0], read_buf[1], read_buf[2]);

	if ((read_buf[0] == id[0]) &&
	    (read_buf[1] == id[1]) &&
	    (read_buf[2] == id[2]))
		ret = 1;
	else
		ret = 0;

	return ret;
#else
	return 0;
#endif
}

static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{
	unsigned int brightness_level = 0;
	bool ret = 0;

	if (sleepout_flag == TRUE) {
		/* The first frame data delay 30Ms, and then turn on the backlight. */
		MDELAY(30);
		sleepout_flag = FALSE;
	}

	ret = get_hbm_status();
	if (ret == TRUE) {
		level = MAX_BRIGHTNESS;
		brightness_level = level;
		bl_level[0].para_list[0] = (brightness_level & 0x700) >> SHITF_EIGHT;
		bl_level[0].para_list[1] = (brightness_level & 0xFF);
		LCM_LOGI("%s, lcm ili7807s backlight: brightness_level = %d\n", __func__, brightness_level);
		push_table(handle, bl_level, ARRAY_SIZE(bl_level), 1);
		return;
	} else {
		if ((level <= MAX_BRIGHTNESS) && (level > MIN_BRIGHTNESS))
			level = (unsigned int)level * NORMAL_MODE_RATIO / TEN;
		else if ((level > 0) && (level <= MIN_BRIGHTNESS))
			level = MIN_BRIGHTNESS - HBM_DIFF;
		else
			level = 0;
	}
        
	brightness_level = level;
	bl_level[0].para_list[0] = (brightness_level & 0x700) >> SHITF_EIGHT;
	bl_level[0].para_list[1] = (brightness_level & 0xFF);

	LCM_LOGI("%s, lcm ili7807s backlight: brightness_level = %d\n", __func__, brightness_level);
	push_table(handle, bl_level, ARRAY_SIZE(bl_level), 1);
}

static void lcm_update(unsigned int x,
	unsigned int y, unsigned int width, unsigned int height)
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

#ifdef LCM_SET_DISPLAY_ON_DELAY
	lcm_set_display_on();
#endif

	data_array[0] = 0x00053902;
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00053902;
	data_array[1] = (y1_MSB << 24) | (y0_LSB << 16) | (y0_MSB << 8) | 0x2b;
	data_array[2] = (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);
}
static unsigned int lcm_compare_id(void)
{
	unsigned int id = 0;
	unsigned char buffer[1];

	read_reg_v2(0xDA, buffer, 1);
	id = buffer[0];
        LCM_LOGI("%s,lcm ili7807s id = %x\n", __func__, id);

	if (id == LCM_ID_ILI9883A)
		return 1;
	else
		return 0;
}

struct LCM_DRIVER ili7807s_hdp_dsi_vdo_tm_lcm_drv = {
	.name = "ili7807s_hdp_dsi_vdo_tm",
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
	.update = lcm_update,
};
