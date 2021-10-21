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
#include "lcm_define.h"
#include "disp_dts_gpio.h"
#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>

#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_pm_ldo.h>
#include <mach/mt_gpio.h>

#ifndef CONFIG_FPGA_EARLY_PORTING
#include <cust_gpio_usage.h>
#include <cust_i2c.h>
#endif

#endif
#endif

#define LCM_ID_0			0x12
#define LCM_ID_1 			0x08
#define LCM_ID_2	 		0x2C
#define LCM_ID_3			0x93

static struct LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define MDELAY(n)		(lcm_util.mdelay(n))
#define UDELAY(n)		(lcm_util.udelay(n))

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
#define LCM_DSI_CMD_MODE                                    0
#define FRAME_WIDTH                                     (1080)
#define FRAME_HEIGHT                                    (2400)

/* physical size in um */
#define LCM_PHYSICAL_WIDTH (67068)
#define LCM_PHYSICAL_HEIGHT (149040)
#define LCM_DENSITY	(420)


#define REGFLAG_DELAY		0xFFFC
#define REGFLAG_UDELAY	0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD
#define REGFLAG_RESET_LOW	0xFFFE
#define REGFLAG_RESET_HIGH	0xFFFF

static struct LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif
unsigned int area = 0;

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{REGFLAG_DELAY, 10, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 150, {} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table init_setting[] = {
{0xfe, 0x01, {0xD2}},
{0x74, 0x01, {0x00}},
{0x75, 0x01, {0x00}},
{0xfe, 0x01, {0x00}},
{0x5e, 0x01, {0x01}},
{0xc2, 0x01, {0x03}},
{0x35, 0x01, {0x00}},
{0x51, 0x02, {0x07,0xff}},
{0xfe, 0x01, {0x00}},
{0x53, 0x01, {0x28}},
{0xfe, 0x01, {0x60}},
{0xc3, 0x01, {0x08}},
{0xfe, 0x01, {0x20}},
{0xb8, 0x01, {0x08}},
{0xb9, 0x01, {0x09}},
{0xba, 0x01, {0x09}},
{0xfe, 0x01, {0x00}},
{0x11, 0x00, {}},
{REGFLAG_DELAY, 120, {}},
/* Display On*/
{0x29, 0x00, {}},
{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table bl_level[] = {
	{0xfe, 0x01, {0x00}},
	{0x51,2,{0x07,0xff}},
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table level_00_1e[] = {
	{0xfe,1,{0x20}},
	{0xb8,1,{0x1d}},
	{0xb9,1,{0x1d}},
	{0xba,1,{0x1d}},
	{0xfe,1,{0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table level_1f_1fd[] = {
	{0xfe,1,{0x20}},
	{0xb8,1,{0x12}},
	{0xb9,1,{0x12}},
	{0xba,1,{0x12}},
	{0xfe,1,{0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table level_1fe_2ff[] = {
	{0xfe,1,{0x20}},
	{0xb8,1,{0x0d}},
	{0xb9,1,{0x0d}},
	{0xba,1,{0x0d}},
	{0xfe,1,{0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table level_300_3ff[] = {
	{0xfe,1,{0x20}},
	{0xb8,1,{0x0b}},
	{0xb9,1,{0x0b}},
	{0xba,1,{0x0b}},
	{0xfe,1,{0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table level_400_4ff[] = {
	{0xfe,1,{0x20}},
	{0xb8,1,{0x0a}},
	{0xb9,1,{0x0a}},
	{0xba,1,{0x0a}},
	{0xfe,1,{0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table level_500_5ff[] = {
	{0xfe,1,{0x20}},
	{0xb8,1,{0x09}},
	{0xb9,1,{0x09}},
	{0xba,1,{0x09}},
	{0xfe,1,{0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table level_600_fff[] = {
	{0xfe,1,{0x20}},
	{0xb8,1,{0x08}},
	{0xb9,1,{0x09}},
	{0xba,1,{0x09}},
	{0xfe,1,{0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static void push_table(struct LCM_setting_table *table,
		unsigned int count,
		unsigned char force_update)
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
			dsi_set_cmdq_V2(cmd, table[i].count,
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
	params->lcm_if              = LCM_INTERFACE_DSI0;
	params->lcm_cmd_if          = LCM_INTERFACE_DSI0;
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->physical_width = LCM_PHYSICAL_WIDTH/1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT/1000;
	params->physical_width_um = LCM_PHYSICAL_WIDTH;
	params->physical_height_um = LCM_PHYSICAL_HEIGHT;

	params->density		   = LCM_DENSITY;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
#else
	params->dsi.mode = BURST_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
#endif
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

	params->dsi.vertical_sync_active = 4;
	params->dsi.vertical_backporch = 12;
	params->dsi.vertical_frontporch = 16;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 2; //2
	params->dsi.horizontal_backporch = 40; //40
	params->dsi.horizontal_frontporch = 40; //40
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_disable = 1;
#ifndef CONFIG_FPGA_EARLY_PORTING
	params->dsi.PLL_CLOCK = 510;    /* this value must be in MTK suggested table */
#else
	params->dsi.pll_div1 = 0;
	params->dsi.pll_div2 = 0;
	params->dsi.fbk_div = 0x1;
#endif

	params->dsi.cont_clock = 0;
	// params->dsi.clk_lp_per_line_enable = 1;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
}

static void lcm_init_power(void)
{

	pr_info("[LCM]%s\n",__func__);
}


static void lcm_suspend_power(void)
{
	pr_info("[LCM]%s\n",__func__);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT0);
	MDELAY(2);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_VCI_OUT0);
	MDELAY(2);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_VDDI_OUT0);
	MDELAY(5);
}

static void lcm_resume_power(void)
{
	pr_info("[LCM]%s\n",__func__);

}

static void lcm_init(void)
{
	pr_info("[LCM]%s\n",__func__);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_VDDI_OUT1);
	MDELAY(10);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_VCI_OUT1);
	MDELAY(10);

	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT0);
	MDELAY(10);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
	MDELAY(10);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT0);
	MDELAY(10);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
	MDELAY(15);
	push_table(init_setting,
		sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	pr_info("[LCM]%s rm692cx\n",__func__);
	push_table(lcm_suspend_setting,
		sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table),
			1);
}

static void lcm_resume(void)
{
	pr_info("[LCM]%s\n",__func__);
	lcm_init();
}

static void lcm_update(unsigned int x, unsigned int y, unsigned int width,
		unsigned int height)
{
#if LCM_DSI_CMD_MODE
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
#endif
}

/*
struct LCM_setting_table switch_table_page[] = {
	{ 0xF0, 2, {0x5A,0x59} }
};
*/

static unsigned int lcm_compare_id(void)
{
	unsigned char buffer[4];
	unsigned int array[16];

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
	MDELAY(20);

	array[0] = 0x00043700;	// read id return four byte
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xA1, buffer, 4);
	pr_info("%s:rm692cx_gt9885: id=0x%02x%02x%02x%02x\n", __func__, buffer[0], buffer[1], buffer[2], buffer[3]);
	if (LCM_ID_0 == buffer[0] && LCM_ID_1 == buffer[1] && LCM_ID_2 == buffer[2] && LCM_ID_3 == buffer[3]) {
		pr_info("%s:kernel:rm692cx_gt9885 reg A1 matched.\n", __func__);
		return 1;
	}
	else {
		pr_info("%s:rm692cx_gt9885 reg A1 unmatched.\n", __func__);
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

	read_reg_v2(0x53, buffer, 1);

	if (buffer[0] != 0x24) {
		pr_debug("[LCM][LCM ERROR] [0x53]=0x%02x\n", buffer[0]);
		return TRUE;
	}
	pr_debug("[LCM][LCM NORMAL] [0x53]=0x%02x\n", buffer[0]);
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

	pr_debug("[LCM]ATA check size = 0x%x,0x%x,0x%x,0x%x\n",
			x0_MSB, x0_LSB, x1_MSB, x1_LSB);
	data_array[0] = 0x0005390A;	/* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	/* read id return two byte,version and id */
	data_array[0] = 0x00043700;
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
	unsigned int level_new = 0;

	pr_info("[LCM]%s, rm692cx_gt9885: backlight value = %d \n", __func__, level);

	bl_level[1].para_list[0] = (level&0x700)>>8;
	bl_level[1].para_list[1] = (level&0xFF);
	level_new=((bl_level[1].para_list[0] << 8) & 0x7ff) | (bl_level[1].para_list[1]  & 0x7ff);
	push_table(bl_level,
			sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
	pr_info("[LCM]%s,level_new is %x\n", __func__,level_new);

	//mura
		switch(level_new)
		{
			case 0x00 ... 0x1e :
					{
						if(area == 1){
							break;
						}
						else
						{
							MDELAY(20);
							push_table(level_00_1e,
							sizeof(level_00_1e)/sizeof(struct LCM_setting_table),1);
							pr_info("[LCM]%s,log check is level_00_1e\n", __func__);
							area = 1;
						}
						break;
					}

			case 0x1f ... 0x1fd :
					{
						if(area == 2){
							break;
						}
						else{
							MDELAY(20);
							push_table(level_1f_1fd,
							sizeof(level_1f_1fd)/sizeof(struct LCM_setting_table),1);
							pr_info("[LCM]%s,log check is level_1f_1fd\n", __func__);
							area = 2;
						}
						break;
					}

			case 0x1fe ... 0x2ff :
					{
						if(area == 3){
							break;
						}
						else{
							MDELAY(20);
							push_table(level_1fe_2ff,
							sizeof(level_1fe_2ff)/sizeof(struct LCM_setting_table),1);
							pr_info("[LCM]%s,log check is level_1fe_2ff\n", __func__);

							area = 3;
						}
						break;
					}

			case 0x300 ... 0x3ff:
					{
						if(area == 4){
							break;
						}
						else{
							MDELAY(20);
							push_table(level_300_3ff,
							sizeof(level_300_3ff)/sizeof(struct LCM_setting_table),1);
							pr_info("[LCM]%s,log check is level_300_3ff\n", __func__);

							area = 4;
						}
						break;
					}

			case 0x400 ... 0x4ff:
					{
						if(area == 5){
							break;
						}
						else{
							MDELAY(20);
							push_table(level_400_4ff,
							sizeof(level_400_4ff)/sizeof(struct LCM_setting_table),1);
							pr_info("[LCM]%s,log check is level_400_4ff\n", __func__);

							area = 5;
						}
						break;
					}

			case 0x500 ... 0x5ff:
					{
						if(area == 6){
							break;
						}
						else{
							MDELAY(20);
							push_table(level_500_5ff,
							sizeof(level_500_5ff)/sizeof(struct LCM_setting_table),1);
							pr_info("[LCM]%s,log check is level_500_5ff\n", __func__);

							area = 6;
						}
						break;
					}

			case 0x600 ... 0xfff:
					{
						if(area == 7){
							break;
						}
						else{
							MDELAY(20);
							push_table(level_600_fff,
							sizeof(level_600_fff)/sizeof(struct LCM_setting_table),1);
							pr_info("[LCM]%s,log check is level_600_fff\n", __func__);

							area = 7;
						}
						break;
					}

			default:
					break;
		}
}

static void *lcm_switch_mode(int mode)
{
#ifndef BUILD_LK
	/* customization: 1. V2C config 2 values, */
	/* C2V config 1 value; 2. config mode control register */
	if (mode == 0) {	/* V2C */
		lcm_switch_mode_cmd.mode = CMD_MODE;
		/* mode control addr */
		lcm_switch_mode_cmd.addr = 0xBB;
		/* enabel GRAM firstly, ensure writing one frame to GRAM */
		lcm_switch_mode_cmd.val[0] = 0x13;
		/* disable video mode secondly */
		lcm_switch_mode_cmd.val[1] = 0x10;
	} else {		/* C2V */
		lcm_switch_mode_cmd.mode = SYNC_PULSE_VDO_MODE;
		lcm_switch_mode_cmd.addr = 0xBB;
		/* disable GRAM and enable video mode */
		lcm_switch_mode_cmd.val[0] = 0x03;
	}
	return (void *)(&lcm_switch_mode_cmd);
#else
	return NULL;
#endif
}

struct LCM_DRIVER mipi_mot_vid_visionox_rm692cx_gt9885_fhd_643_lcm_drv = {
	.name = "mipi_mot_vid_visionox_rm692cx_gt9885_fhd_643",
	.supplier = "visionox_rm692cx",
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
};
