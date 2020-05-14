/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2015. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */
#define LOG_TAG "LCM-hx83102d-truly"

#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#include "tpd.h"
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

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif


static const unsigned int BL_MIN_LEVEL = 20;
static struct LCM_UTIL_FUNCS lcm_util;

#define LCM_ID_DA_MP		0x00
#define LCM_ID_DC_MP		0x67

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))
#define MDELAY(n)       (lcm_util.mdelay(n))
#define UDELAY(n)       (lcm_util.udelay(n))


/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */
#ifndef BUILD_LK
#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#endif
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
#ifdef BUILD_LK
#define LCD_BIAS_ADDR 0x3E
#ifndef GPIO_LCD_BIAS_ENP_PIN
#define GPIO_LCD_BIAS_ENP_PIN (GPIO172|0x80000000)
#endif

#ifndef GPIO_LCD_BIAS_ENN_PIN
#define GPIO_LCD_BIAS_ENN_PIN (GPIO173|0x80000000)
#endif

#ifndef GPIO_LCD_RST_PIN
#define GPIO_LCD_RST_PIN (GPIO45|0x80000000)
#endif

#ifndef GPIO_CTP_RST_PIN
#define GPIO_CTP_RST_PIN (GPIO174|0x80000000)
#endif
#else
#define set_gpio_lcd_enp(cmd) \
	lcm_util.set_gpio_lcd_enp_bias(cmd)
#define set_gpio_lcd_enn(cmd) \
	lcm_util.set_gpio_lcd_enn_bias(cmd)
#endif
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
#define LCM_DSI_CMD_MODE             0
#define FRAME_WIDTH                 (720)
#define FRAME_HEIGHT                (1600)
#define LCM_DENSITY					(320)

#define LCM_PHYSICAL_WIDTH          (67932)
#define LCM_PHYSICAL_HEIGHT         (150960)
#define REGFLAG_DELAY       		0xFFFC
#define REGFLAG_UDELAY  			0xFFFB
#define REGFLAG_END_OF_TABLE        0xFFFD
#define REGFLAG_RESET_LOW   		0xFFFE
#define REGFLAG_RESET_HIGH  		0xFFFF

#ifndef BUILD_LK
extern int NT50358A_write_byte(unsigned char addr, unsigned char value);
extern volatile int gesture_dubbleclick_en;
//static struct LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;
#else
#define I2C_I2C_LCD_BIAS_CHANNEL 6    /* for I2C channel 0 */
static struct mt_i2c_t NT50358A_i2c;
#endif

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
	{0x28,0x01, {0x00} },
	{REGFLAG_DELAY, 20, {} },
	{0x10, 0x01, {0x00} },
	{REGFLAG_DELAY, 150, {} },
};

static struct LCM_setting_table init_setting[] = {
{0x11, 1, {0x00}},
{REGFLAG_DELAY, 15, {}},

{0xB9, 3, {0x83,0x10,0x2D}},

{0xB1, 11, {0x22,0x44,0x31,0x31,0x33,0x42,0x4D,0x2F,0x08,0x08,0x08}},

{0xB2, 14, {0x00,0x00,0x06,0x40,0x00,0x0D,0x20,0x3D,0x13,0x00,0x00,0x01,0x85,0xA0}},

{0xB4, 14, {0x08,0x1B,0x08,0x1B,0x02,0x3A,0x02,0x3A,0x03,0xFF,0x05,0x20,0x00,0xFF}},

{0xCC, 1, {0x02}},

{0xD3, 25, {0x00,0x00,0x01,0x01,0x00,0x00,0x00,0x7F,0x00,0x77,0x35,0x0C,0x0C,0x00,0x00,0x32,0x10,0x04,0x00,0x04,0x54,0x16,0x52,0x06,0x52}},
{REGFLAG_DELAY, 5, {}},

{0xD5, 44, {0x25,0x24,0x19,0x19,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x0F,0x0E,0x07,0x06,0x0D,0x0C,0x05,0x04,0x0B,0x0A,0x03,0x02,0x09,0x08,0x01,0x00,0x21,0x20,0x21,0x20,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18}},
{REGFLAG_DELAY, 5, {}},

{0xD6, 44, {0x20,0x21,0x18,0x18,0x18,0x18,0x18,0x18,0x19,0x19,0x18,0x18,0x18,0x18,0x00,0x01,0x08,0x09,0x02,0x03,0x0A,0x0B,0x04,0x05,0x0C,0x0D,0x06,0x07,0x0E,0x0F,0x24,0x25,0x24,0x25,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18}},
{REGFLAG_DELAY, 5, {}},

{0xE7, 29, {0xFF,0x10,0x02,0x02,0x00,0x00,0x14,0x14,0x00,0x07,0x0F,0x84,0x3C,0x84,0x32,0x32,0x00,0x00,0x00,0x00,0x00,0x10,0x68,0x10,0x10,0x1D,0xB9,0x23,0xB9}},

{0xBD, 1, {0x01}},

{0xE7, 7, {0x01,0x40,0x01,0x40,0x13,0x40,0x14}},

{0xBD, 1, {0x02}},

{0xE7, 16, {0x00,0xFE,0x00,0xFE,0x00,0x00,0x02,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

{0xD8, 12, {0xFF,0xFF,0xFF,0xFF,0xFF,0xF0,0xFF,0xFF,0xFF,0xFF,0xFF,0xF0}},

{0xBD, 1, {0x03}},

{0xD8, 24, {0xAA,0xAA,0xAA,0xAA,0xAA,0xA0,0xAA,0xAA,0xAA,0xAA,0xAA,0xA0,0xFF,0xFF,0xFF,0xFF,0xFF,0xF0,0xFF,0xFF,0xFF,0xFF,0xFF,0xF0}},

{0xBD, 1, {0x00}},

//{0xB6, 2, {0x41,0x41}},

//{0xE0, 46, {0x00,0x1A,0x3B,0x45,0x4F,0x99,0xB2,0xB8,0xBE,0xB8,0xCE,0xD0,0xD1,0xDB,0xD4,0xD6,0xDB,0xE9,0xE5,0x6F,0x75,0x7C,0x7F,0x00,0x1A,0x3B,0x45,0x4F,0x99,0xB2,0xB8,0xBE,0xB8,0xCE,0xD0,0xD1,0xDB,0xD4,0xD6,0xDB,0xE9,0xE5,0x6F,0x75,0x7C,0x7F}},
//{REGFLAG_DELAY, 5, {}},

{0xBA, 19, {0x70,0x23,0xA8,0x93,0xB2,0xC0,0xC0,0x01,0x10,0x00,0x00,0x00,0x0D,0x3D,0x82,0x77,0x04,0x01,0x04}},

{0xBF, 7, {0xFC,0x00,0x04,0x9E,0xF6,0x00,0x5D}},

{0xCB, 5, {0x00,0x13,0x00,0x02,0x66}},

{0xBD, 1, {0x01}},

{0xCB, 1, {0x01}},

{0xBD, 1, {0x02}},

{0xB4, 8, {0x42,0x00,0x33,0x00,0x33,0x88,0xB3,0x33}},

{0xB1, 3, {0x7F,0x03,0xF5}},

{0xBD, 1, {0x00}},

{0x00, 1, {0x00}},

{0x35, 1, {0x00}},

{0x51, 0x02,{0x00,0x00}},
{0x53, 0x01,{0x24}},
{0x55, 0x01,{0x00}},

{0x29, 1, {0x00}},
{REGFLAG_DELAY, 55, {}},
{REGFLAG_END_OF_TABLE, 0x00, {} }
};
static struct LCM_setting_table bl_level[] = {
	{0x51, 0x02,{0x0F,0xFF}},
	{REGFLAG_DELAY, 1, {} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static void push_table(void *cmdq, struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
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
				LCM_LOGI("cmd=%x, %x\n",cmd,table[i].para_list[0]);
				dsi_set_cmdq_V22(cmdq, cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}


static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	LCM_LOGI("%s: \n",__func__);
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
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
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
	params->dsi.vertical_sync_active = 2; //old is 2,now is 4
	params->dsi.vertical_backporch = 16; //old is 8,now is 100
	params->dsi.vertical_frontporch = 190; //old is 24,now is 124
	params->dsi.vertical_active_line = FRAME_HEIGHT;
	params->dsi.horizontal_sync_active = 11; //old is 20,now is 8
	params->dsi.horizontal_backporch = 12;//old is 60,now is 12
	params->dsi.horizontal_frontporch = 12;//old is 60,now is 16
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.PLL_CLOCK = 290;    /* FrameRate = 60Hz */ /* this value must be in MTK suggested table */
	params->dsi.ssc_disable = 1;

	params->dsi.cont_clock = 0;
	params->dsi.clk_lp_per_line_enable = 1;

	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9D;
	params->dsi.lcm_esd_check_table[1].cmd = 0x09;
	params->dsi.lcm_esd_check_table[1].count = 3;
	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x80;
	params->dsi.lcm_esd_check_table[1].para_list[1] = 0x73;
	params->dsi.lcm_esd_check_table[1].para_list[2] = 0x06;
}
#ifdef BUILD_LK
static int NT50358A_write_byte(kal_uint8 addr, kal_uint8 value)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;

	write_data[0] = addr;
	write_data[1] = value;

	NT50358A_i2c.id = I2C_I2C_LCD_BIAS_CHANNEL; /* I2C2; */
	/* Since i2c will left shift 1 bit, we need to set FAN5405 I2C address to >>1 */
	NT50358A_i2c.addr = LCD_BIAS_ADDR;
	NT50358A_i2c.mode = ST_MODE;
	NT50358A_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&NT50358A_i2c, write_data, len);
	/* printf("%s: i2c_write: ret_code: %d\n", __func__, ret_code); */

	return ret_code;
}
#endif

static void i2c_send_data_lcd(unsigned char cmd, unsigned char data)
{
	int ret = 0;
	ret = NT50358A_write_byte(cmd, data);
	if (ret < 0)
		LCM_LOGI("----cmd=%0x--i2c write error----\n", cmd);
	else
		LCM_LOGI("---cmd=%0x--i2c write success----\n", cmd);
}

static void lcm_reset(void)
{
	if (!gesture_dubbleclick_en) {
		SET_RESET_PIN(1);
		MDELAY(2);//T2
		SET_RESET_PIN(0);
		MDELAY(2);//T3
		SET_RESET_PIN(1);
		//tpd_gpio_output(0, 1);
		MDELAY(50);//T4
	}

	LCM_LOGI("%s: lcm reset done\n",__func__);
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
	unsigned char data = 0x13;  //up to +/-5.9V
	LCM_LOGI("%s: start init\n",__func__);

	if (!gesture_dubbleclick_en) {
		SET_RESET_PIN(0);
		//tpd_gpio_output(0, 0);
		MDELAY(1);

		set_gpio_lcd_enp(1);
		MDELAY(6);//for bias IC and tr2
		i2c_send_data_lcd(cmd, data);

		MDELAY(2);//T1
		set_gpio_lcd_enn(1);
		MDELAY(7);//for bias IC and tr2
		cmd = 0x01;
		data = 0x13;
		i2c_send_data_lcd(cmd, data);
	}
	lcm_reset();

	push_table(NULL, init_setting,sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
	LCM_LOGI("%s: init done\n",__func__);
}

static void lcm_suspend(void)
{
#ifndef MACH_FPGA

	LCM_LOGI("%s, lcm_suspend start\n", __func__);
	push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
	if (!gesture_dubbleclick_en) {
		set_gpio_lcd_enn(0);
		MDELAY(2);
		set_gpio_lcd_enp(0);
	}
#endif
	LCM_LOGI("%s, lcm_suspend done\n", __func__);

}

static void lcm_resume(void)
{
	LCM_LOGI("%s, start\n",__func__);
	lcm_init();
	LCM_LOGI("%s, done\n",__func__);
}


static unsigned int lcm_ata_check(unsigned char *buffer)
{
	return 0;
}

static void lcm_setbacklight(void *handle, unsigned int level)
{
	if (level > 255)
		level = 255;

	bl_level[0].para_list[0] = (level & 0xF0)>>4;
	bl_level[0].para_list[1] = (level & 0x0F)<<4;
	LCM_LOGI("%s, backlight set level = %d \n", __func__, level);
	push_table(handle, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}

static unsigned int lcm_compare_id(void)
{
	unsigned int id_da = 0, id_dc = 0;
	unsigned char buffer[2];
	unsigned int array[16];

	SET_RESET_PIN(1);
	MDELAY(3);

	array[0] = 0x00023700;  /* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xDA, buffer, 1);
	id_da = buffer[0];         /* we only need ID */
	read_reg_v2(0xDC, buffer, 1);
	id_dc = buffer[0];         /* we only need ID */

	LCM_LOGI("%s, vendor id_da=0x%02x, id_dc=0x%02x \n", __func__, id_da,id_dc);

	if ((id_da == LCM_ID_DA_MP && id_dc == LCM_ID_DC_MP))
		return 1;
	else
		return 0;
}


struct LCM_DRIVER hx83102d_hdplus_dsi_vdo_truly_6528_lcm_drv = {
	.name = "hx83102d_hdplus_dsi_vdo_truly_6528",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id = lcm_compare_id,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.set_backlight_cmdq = lcm_setbacklight,
	.ata_check = lcm_ata_check,

};
