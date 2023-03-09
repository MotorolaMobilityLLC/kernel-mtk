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

#include "mtkfb_params.h"

#define LCM_LOGI(fmt, args...)  pr_info("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)

#define LCM_ID_0			0x08
#define LCM_ID_1 			0x01
#define LCM_ID_2	 		0x67
#define LCM_ID_3 			0xA3

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

#define ALS_BL_MAX_BRIGHTNESS			1456	//bl max level of upper layer
#define LCM_BL_MAX_LEVEL_DEFAULT		1456	//DVT2, PVT
#define LCM_BL_MAX_LEVEL_MIN			1000	//bl max level min check
#define LCM_BL_MAX_LEVEL_MAX			2047
/*
#define LCM_BL_MAX_LEVEL_DVT1			1820
#define LCM_BL_MAX_LEVEL_EVT			1750
*/

static unsigned int hbm_ver, bl_max_level = LCM_BL_MAX_LEVEL_DEFAULT;
static bool need_get_dt = 1;
//should keep consisent with lcm,hbm-vev in dts
enum lcm_hbm_hwrev {
	LCM_HBM_DEFAULT = 0, 	//dvt2, pvt
	LCM_HBM_DVT1,		//dvt1
	LCM_HBM_EVT		//evt
};

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
	{REGFLAG_DELAY, 80, {}},
};

static struct LCM_setting_table init_setting_vdo[] = {
	{0XFF, 0x01, {0X10}},
	{0XFB, 0x01, {0X01}},
	{0XB0, 0x01, {0X00}},
	{0XC0, 0x01, {0X00}},

	{0XFF, 0x01, {0XF0}},
	{0XFB, 0x01, {0X01}},
	{0X20, 0x01, {0X80}},
	{0XD2, 0x01, {0X50}},

	{0XFF, 0x01, {0X2B}},
	{0XFB, 0x01, {0X01}},
	{0XB7, 0x01, {0X2C}},
	{0XB8, 0x01, {0X1E}},
	{0XC0, 0x01, {0X01}},

	{0xFF, 0x01, {0x24}},
	{0XFB, 0x01, {0X01}},
	{0X32, 0x01, {0X09}},

	{0xFF, 0x01, {0x25}},
	{0XFB, 0x01, {0X01}},
	{0X18, 0x01, {0X21}},

	{0XFF, 0x01, {0X10}},
	{0XFB, 0x01, {0X01}},

	{0X35, 0x01, {0X00}},
	{0x53, 0x01, {0x2C}},
	{0x55, 0x01, {0x01}},
	{0x68, 0x02, {0x03,0x01}},

	{0x11, 0x01, {0x00}},
	{REGFLAG_DELAY,120,{}},
	{0x29, 0x01, {0x00}},
	{REGFLAG_DELAY, 20, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}},

};


static struct LCM_setting_table bl_level[] = {
	{0XFF, 0x01, {0X10}},
	{0XFB, 0x01, {0X01}},
	{0x51, 0x02, {0x04, 0xFF}},
	{ REGFLAG_END_OF_TABLE, 0x00, {} },
};

static struct LCM_setting_table lcm_cabc_setting_ui[] = {
	{0xFF, 1, {0x10} },
	{0xFB, 1, {0x01} },
	{0x55, 1, {0x01} },
};

static struct LCM_setting_table lcm_cabc_setting_mv[] = {
	{0xFF, 1, {0x10} },
	{0xFB, 1, {0x01} },
	{0x55, 1, {0x03} },
};

static struct LCM_setting_table lcm_cabc_setting_disable[] = {
	{0xFF, 1, {0x10} },
	{0xFB, 1, {0x01} },
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

//HBM START, keep consistent with dts lcm,max-level / LCM_BL_MAX_LEVEL_*
//dvt2, pvt
static struct LCM_setting_table lcm_hbm_off[] = {
	{0XFF, 0x01, {0X10}},
	{0XFB, 0x01, {0X01}},
	{0x51, 0x02, {0x05, 0xB0} },
};

//dvt1
static struct LCM_setting_table lcm_hbm_off_dvt1[] = {
	{0XFF, 0x01, {0X10}},
	{0XFB, 0x01, {0X01}},
	{0x51, 0x02, {0x07, 0x1C} },
};

//evt
static struct LCM_setting_table lcm_hbm_off_evt[] = {
	{0XFF, 0x01, {0X10}},
	{0XFB, 0x01, {0X01}},
	{0x51, 0x02, {0x06, 0xD6} },
};

static struct LCM_setting_table lcm_hbm_on[] = {
	{0XFF, 0x01, {0X10}},
	{0XFB, 0x01, {0X01}},
	{0x51, 0x02, {0x07, 0xFF} },
};
//HBM END

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
	dfps_params[0].vertical_frontporch = 900;
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

static int lcm_parse_dt() {
	int ret = 0;
    struct device_node *node = NULL;
	char node_name[] = "mediatek,disp_panel";

	LCM_LOGD("tm: enter\n");
    node = of_find_compatible_node(NULL, NULL, node_name);
    if (node) {
        int ret, value;

		ret = of_property_read_u32(node, "lcm,hbm-vev", &value);
		if (!ret && value) {
			hbm_ver = value;
			LCM_LOGI("get lcm,hbm-vev =%d\n", hbm_ver);
		}
		else
			hbm_ver = 0;

#if 1 //get max brightness from dts
		ret = of_property_read_u32(node, "lcm,max-level", &value);
		if (!ret && value >= LCM_BL_MAX_LEVEL_MIN && value <= LCM_BL_MAX_LEVEL_MAX) {
			bl_max_level = value;
			LCM_LOGI("tm: get max-level, bl_max_level=%d\n", bl_max_level);
		}
		else {
			bl_max_level = LCM_BL_MAX_LEVEL_DEFAULT;
			LCM_LOGI("tm: get max-level fail ret=%d, bl_max_level=%d\n", ret, bl_max_level);
		}
#else //set max brightness per panel if need
		switch(hbm_ver) {
			case LCM_HBM_DVT1:
				bl_max_level = LCM_BL_MAX_LEVEL_DVT1;
				break;
			case LCM_HBM_EVT:
				bl_max_level = LCM_BL_MAX_LEVEL_EVT;
				break;
			default:
				bl_max_level = LCM_BL_MAX_LEVEL_DEFAULT;
				break;
		}
		LCM_LOGI("tm: set bl_max_level=%d\n", bl_max_level);
#endif
    } else {
		bl_max_level = LCM_BL_MAX_LEVEL_DEFAULT;
		LCM_LOGI("tm: node not found, set bl_max_level=%d\n", bl_max_level);
    }

	return ret;
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
	params->dsi.vertical_backporch = 18;
	params->dsi.vertical_frontporch = 54;
	//params->dsi.vertical_frontporch_for_low_power = 540;/*disable dynamic frame rate*/
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 4;
	params->dsi.horizontal_backporch = 68;
	params->dsi.horizontal_frontporch = 68;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_disable = 1;

#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 419;	/* this value must be in MTK suggested table */
#else
	params->dsi.PLL_CLOCK = 419;	/* this value must be in MTK suggested table */
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

	if (need_get_dt) {
		//get dt info when first enter
		lcm_parse_dt();
		need_get_dt = 0;
	}
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
	MDELAY(25);
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
	SET_RESET_PIN(0);
	MDELAY(3);
	SET_RESET_PIN(1);
	MDELAY(3);
	SET_RESET_PIN(0);
	MDELAY(5);

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

	LCM_LOGI("%s,csot_nt id=0x%02x%02x%02x%02x\n", __func__,buffer[0], buffer[1],buffer[2],buffer[3]);
	if (LCM_ID_0 == buffer[0] && LCM_ID_1 == buffer[1] && LCM_ID_2 == buffer[2] && LCM_ID_3 == buffer[3]) {
		LCM_LOGI("%s: csot_nt 653 matched\n", __func__);
		return 1;
	}
	else {
		LCM_LOGI("%s: csot_nt 653 not matched\n", __func__);
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

	if (bl_max_level < LCM_BL_MAX_LEVEL_MIN || bl_max_level > LCM_BL_MAX_LEVEL_MAX) {
		//bl_max_level recheck
		LCM_LOGI("resume:tm: abnormal bl_max_level=%d\n, bl_max_level");
		lcm_parse_dt();
	}

	//trans bl level for HBM case
	bl_lvl = level*bl_max_level/ALS_BL_MAX_BRIGHTNESS;
	//avoid black when level > 0 and low
	if (level > 0 && !bl_lvl)
		bl_lvl = 1;

	//for 11bit
	bl_level[2].para_list[0] = (bl_lvl&0x700)>>8;
	bl_level[2].para_list[1] = (bl_lvl&0xFF);

	if (bl_lvl == level)
		LCM_LOGI("%s:csot_nt:level=bl_lvl=%d, para[0]=0x%x,para[1]=0x%x\n",__func__, bl_lvl, bl_level[2].para_list[0],bl_level[2].para_list[1]);
	else
		LCM_LOGI("%s:csot_nt:level=%d, bl_lvl=%d, para[0]=0x%x,para[1]=0x%x\n",__func__, level, bl_lvl, bl_level[2].para_list[0],bl_level[2].para_list[1]);
	push_table(handle, bl_level,
		sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_set_cmdq(void *handle, unsigned int *lcm_cmd,
		unsigned int *lcm_count, unsigned int *lcm_value)
{
	switch(*lcm_cmd) {
		case PARAM_HBM:
			if(*lcm_value) {
				pr_info("%s: handle HBM on", __func__);
				push_table(handle, lcm_hbm_on, ARRAY_SIZE(lcm_hbm_on), 1);
			}
			else {
				switch(hbm_ver) {
					case LCM_HBM_DVT1:
						//dvt1
						push_table(handle, lcm_hbm_off_dvt1, ARRAY_SIZE(lcm_hbm_off_dvt1), 1);
						pr_info("%s:v1: handle HBM off", __func__);
						break;
					case LCM_HBM_EVT:
						//evt
						push_table(handle, lcm_hbm_off_evt, ARRAY_SIZE(lcm_hbm_off_evt), 1);
						pr_info("%s:v2: handle HBM off", __func__);
						break;
					default:
						//dvt2, pvt
						push_table(handle, lcm_hbm_off, ARRAY_SIZE(lcm_hbm_off), 1);
						pr_info("%s: handle HBM off", __func__);
						break;
				}
			}
			break;
		case PARAM_CABC:
			if (*lcm_value >= CABC_MODE_NUM) {
				pr_info("%s: invalid CABC mode:%d out of CABC_MODE_NUM:", *lcm_value, CABC_MODE_NUM);
				return;
			}
			else {
				unsigned int cmd_num = lcm_cabc_settings[*lcm_value].cmd_num;
				pr_info("%s: set CABC mode=%d, cmd_num=%d", __func__, *lcm_value, cmd_num);
				push_table(handle, lcm_cabc_settings[*lcm_value].cabc_cmds, cmd_num, 1);
			}
			break;
		default:
			pr_err("%s,csot_nt cmd:%d, unsupport\n", __func__, *lcm_cmd);
			break;
	}

	pr_debug("%s,csot_nt cmd:%d, value = %d done\n", __func__, *lcm_cmd, *lcm_value);
}

struct LCM_DRIVER mipi_mot_vid_csot_nt36672n_hdp_653_lcm_drv = {
	.name = "mipi_mot_vid_csot_nt36672n_hdp_653",
	.supplier = "csot",
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
	.set_lcm_cmd = lcm_set_cmdq,
	.update = lcm_update,
};
