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
#include "tpd.h"

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

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

#define LCM_ID (0x98)

static const unsigned int BL_MIN_LEVEL = 20;
static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin_gpio((v)))
#define SET_BACKLIGHT_OUT(v)	(lcm_util.set_backlight_out((v)))
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
#include <linux/i2c/ocp2131.h>

/*****************************************************************************
 * Define
 *****************************************************************************/
#endif

static const unsigned char LCD_MODULE_ID = 0x01;
#define LCM_DSI_CMD_MODE								0
#define FRAME_WIDTH									(720)
#define FRAME_HEIGHT									(1520)

#define LCM_PHYSICAL_WIDTH								(64800)
#define LCM_PHYSICAL_HEIGHT								(129600)


#define REGFLAG_DELAY		0xFFFC
#define REGFLAG_UDELAY	0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD
#define REGFLAG_RESET_LOW	0xFFFE
#define REGFLAG_RESET_HIGH	0xFFFF

extern void fts_lcm_update_firmware_work(void);
extern void fts_tp_enter_sleep_mode(void);

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
	{ 0x28, 0, {} },
	{ REGFLAG_DELAY, 30, {} },
	{ 0x10, 0, {} },
	{ REGFLAG_DELAY, 120, {} },
	{ 0x17, 0x01, {0x5A} },
	{ 0x18, 0x01, {0x5A} },
};


static struct LCM_setting_table init_setting_vdo[] = {
	{ 0x41, 0x02, {0x5A, 0x24} },
	{ 0x90, 0x01, {0x5A} },
	{ 0x41, 0x02, {0x5A, 0x0E} },
	{ 0x80, 0x08, {0xFF, 0x01, 0x55, 0x55, 0x32, 0x88, 0x88, 0x1C} },
	{ 0x41, 0x02, {0x5A, 0x19} },
	{ 0xA0, 0x04, {0x05,0x05,0x02,0x00} },
	{ 0x41, 0x02, {0x5A, 0x20} },
	{ 0x80, 0x07, {0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
	{ 0x41, 0x02, {0x5A, 0x24} },
	{ 0x90, 0x01, {0x00} },
	{ 0x41, 0x02, {0x5A, 0x2F} },
	{ 0x19, 0x01, {0x00} },
	{ 0x4C, 0x01, {0x03} },
	{ 0x11, 0, {} },
	{ REGFLAG_DELAY, 120, {} },
	{ 0x29, 0, {} },
	{ REGFLAG_DELAY, 20, {} },
	{ 0x51, 0x01, {0x00} },
	{ 0x52, 0x01, {0x00} },
	{ 0x53, 0x01, {0x2C} },
	{ 0x55, 0x01, {0x01} },
	{ 0x5E, 0x01, {0x01} },
};

static struct LCM_setting_table bl_level[] = {
	{ 0x51, 1, {0xCC} },
	{ REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table cabc_array[] = {
	{ 0x55, 0x01, {0x01} },
	{ REGFLAG_END_OF_TABLE, 0x00, {} }
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
	params->physical_width = LCM_PHYSICAL_WIDTH/1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT/1000;
	//params->physical_width_um = LCM_PHYSICAL_WIDTH;
	//params->physical_height_um = LCM_PHYSICAL_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
	//lcm_dsi_mode = CMD_MODE;
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
	//lcm_dsi_mode = SYNC_PULSE_VDO_MODE;
#endif
	//LCM_LOGI("lcm_get_params lcm_dsi_mode %d\n", lcm_dsi_mode);
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

	params->dsi.vertical_sync_active = 8;
	params->dsi.vertical_backporch = 37;
	params->dsi.vertical_frontporch = 70;
	/* params->dsi.vertical_frontporch_for_low_power = 540; */
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 16;
	params->dsi.horizontal_backporch = 60;
	params->dsi.horizontal_frontporch = 80;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_disable = 1;

#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 250;	/* this value must be in MTK suggested table */
#else
	params->dsi.PLL_CLOCK = 257;	/* this value must be in MTK suggested table */
#endif

	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;

}


static void lcm_init_power(void)
{

}

static void lcm_suspend_power(void)
{

}

static void lcm_resume_power(void)
{

}

static void lcm_init(void)
{

	unsigned char cmd = 0x0;
	unsigned char data = 0xFF;
	int ret = 0;
	cmd = 0x00;
	data = 0x14;

	LCM_LOGD("lcm_init ft8006s\n");

	OCP2131_GPIO_ENP_enable();
	ret = OCP2131_write_bytes(cmd, data);
	if (ret < 0)
		LCM_LOGI("ft8006s----ocp2131----cmd=%0x--i2c write error----\n", cmd);
	else
		LCM_LOGI("ft8006s----ocp2131----cmd=%0x--i2c write success----\n", cmd);

	MDELAY(2);
	OCP2131_GPIO_ENN_enable();
	cmd = 0x01;
	data = 0x14;

	ret = OCP2131_write_bytes(cmd, data);

	if (ret < 0)
		LCM_LOGI("ft8006s----ocp2131----cmd=%0x--i2c write error----\n", cmd);
	else
		LCM_LOGI("ft8006s----ocp2131----cmd=%0x--i2c write success----\n", cmd);

	SET_RESET_PIN(1);
	MDELAY(1);
	tpd_gpio_output(0, 1);
	MDELAY(1);
	tpd_gpio_output(0, 0);
	MDELAY(6);
	SET_RESET_PIN(0);
	MDELAY(6);
	tpd_gpio_output(0, 1);
	MDELAY(1);
	SET_RESET_PIN(1);
	SET_BACKLIGHT_OUT(1);
	MDELAY(35);

	fts_lcm_update_firmware_work();

	push_table(NULL, init_setting_vdo, sizeof(init_setting_vdo) / sizeof(struct LCM_setting_table), 1);
	LCM_LOGI("ft8006s----holitek----lcm mode = vdo mode ----\n");

}

static void lcm_suspend(void)
{
	LCM_LOGD("lcm_suspend\n");
	fts_tp_enter_sleep_mode();
	push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
	/* SET_RESET_PIN(0); */
	SET_BACKLIGHT_OUT(0);
	OCP2131_GPIO_ENN_disable();
	MDELAY(2);
	OCP2131_GPIO_ENP_disable();
}

static void lcm_resume(void)
{
	LCM_LOGD("lcm_resume\n");

	lcm_init();
}

static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{

}

static unsigned int lcm_compare_id(void)
{
	unsigned int id = 0, version_id = 0;
	unsigned char buffer[2];
	unsigned int array[16];
	struct LCM_setting_table switch_table_page1[] = {
		{ 0xFF, 0x03, {0x98, 0x81, 0x01} }
	};
	struct LCM_setting_table switch_table_page0[] = {
		{ 0xFF, 0x03, {0x98, 0x81, 0x00} }
	};

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);

	SET_RESET_PIN(1);
	MDELAY(20);

	push_table(NULL, switch_table_page1, sizeof(switch_table_page1) / sizeof(struct LCM_setting_table), 1);

	array[0] = 0x00023700;	/* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x00, buffer, 1);
	id = buffer[0];		/* we only need ID */

	read_reg_v2(0x01, buffer, 1);
	version_id = buffer[0];

	LCM_LOGI("%s,FT8006S_id=0x%08x,version_id=0x%x\n", __func__, id, version_id);
	push_table(NULL, switch_table_page0, sizeof(switch_table_page0) / sizeof(struct LCM_setting_table), 1);

	if (id == LCM_ID && version_id == 0x25)
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

	read_reg_v2(0x53, buffer, 1);

	if (buffer[0] != 0x24) {
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

	LCM_LOGI("%s,ft8006s backlight: level = %d\n", __func__, level);

	bl_level[0].para_list[0] = level;

	push_table(NULL, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}

static void set_lcm_cmd(void *handle, unsigned int *lcm_cmd, unsigned int *lcm_count,
		unsigned int *lcm_value)
{
	cabc_array[0].para_list[0] = *lcm_value;
	push_table(handle, cabc_array, sizeof(cabc_array) / sizeof(struct LCM_setting_table), 1);
}

static void *lcm_switch_mode(int mode)
{
	return NULL;
}

static void lcm_set_recovery_backlight(void)
{
	push_table(NULL, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}

#if (LCM_DSI_CMD_MODE)
/* partial update restrictions:
 * 1. roi width must be 1080 (full lcm width)
 * 2. vertical start (y) must be multiple of 16
 * 3. vertical height (h) must be multiple of 16
 */
static void lcm_validate_roi(int *x, int *y, int *width, int *height)
{

}
#endif

LCM_DRIVER mipi_mot_vid_holi_720p_622_lcm_drv = {
	.name = "mipi_mot_vid_holi_720p_622",
	.supplier = "holi",
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
	.update = lcm_update,
	.switch_mode = lcm_switch_mode,
#if (LCM_DSI_CMD_MODE)
	.validate_roi = lcm_validate_roi,
#endif
	.set_lcm_cmd = set_lcm_cmd,
	.set_recovery_backlight = lcm_set_recovery_backlight,
};
