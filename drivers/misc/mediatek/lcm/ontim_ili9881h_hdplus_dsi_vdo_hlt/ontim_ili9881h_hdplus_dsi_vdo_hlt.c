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
#define LOG_TAG "LCM-ili9881H HLT"

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

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif


static const unsigned int BL_MIN_LEVEL = 20;
static struct LCM_UTIL_FUNCS lcm_util;
#define LCM_ID_REG_DC (unsigned int)(0x12)
#define LCM_ID_REG_DB (unsigned int)(0x00)
#define LCM_ID_REG_DA (unsigned int)(0x22)
#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))
#define MDELAY(n)       (lcm_util.mdelay(n))
#define UDELAY(n)       (lcm_util.udelay(n))

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
#define set_gpio_lcd_enp(cmd) \
	lcm_util.set_gpio_lcd_enp_bias(cmd)

#define set_gpio_lcd_enn(cmd) \
	lcm_util.set_gpio_lcd_enn_bias(cmd)
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
#define FRAME_WIDTH                                     (720)
#define FRAME_HEIGHT                                    (1600)
#define LCM_DENSITY		(320)

#define LCM_PHYSICAL_WIDTH                  (67932)
#define LCM_PHYSICAL_HEIGHT                    (150960)
#define REGFLAG_DELAY       0xFFFC
#define REGFLAG_UDELAY  0xFFFB
#define REGFLAG_END_OF_TABLE    0xFFFD
#define REGFLAG_RESET_LOW   0xFFFE
#define REGFLAG_RESET_HIGH  0xFFFF
#ifndef BUILD_LK
extern int NT50358A_write_byte(unsigned char addr, unsigned char value);
#endif
//static struct LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;

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
	{REGFLAG_DELAY, 120, {} },
};
#if 1
static struct LCM_setting_table init_setting[] = {
{0xFF,0x03,{0x98,0x81,0x01}},
{0x00,0x01,{0x46}},
{0x01,0x01,{0x16}},
{0x02,0x01,{0x00}},  //10
{0x03,0x01,{0x00}},

{0x08,0x01,{0x80}},
{0x09,0x01,{0x0A}},  //DUMMT CK 12
{0x0a,0x01,{0x71}},
{0x0b,0x01,{0x00}},  //clk keep 10 off 00  10
{0x0c,0x01,{0x00}},  //01
{0x0d,0x01,{0x00}},  //01
{0x0e,0x01,{0x01}},  //10
{0x0f,0x01,{0x01}},   //10

{0x10,0x01,{0x01}},
{0x11,0x01,{0x01}},
{0x12,0x01,{0x01}},

{0x24,0x01,{0x01}},  //01 
{0x25,0x01,{0x0b}},
{0x26,0x01,{0x00}},
{0x27,0x01,{0x00}},

{0x31,0x01,{0x21}},  //STV_C
{0x32,0x01,{0x07}},
{0x33,0x01,{0x01}},
{0x34,0x01,{0x00}},
{0x35,0x01,{0x02}},
{0x36,0x01,{0x07}},
{0x37,0x01,{0x07}},
{0x38,0x01,{0x07}},
{0x39,0x01,{0x07}},
{0x3a,0x01,{0x17}},  //CLK8
{0x3b,0x01,{0x15}},  //CLK6
{0x3c,0x01,{0x07}},
{0x3d,0x01,{0x07}},
{0x3e,0x01,{0x13}},  //CLK4
{0x3f,0x01,{0x11}},  //CLK2
{0x40,0x01,{0x09}},  //STV_A
{0x41,0x01,{0x07}},
{0x42,0x01,{0x07}},
{0x43,0x01,{0x07}},
{0x44,0x01,{0x07}},
{0x45,0x01,{0x07}},
{0x46,0x01,{0x07}},

{0x47,0x01,{0x20}},  //STV_C
{0x48,0x01,{0x07}},
{0x49,0x01,{0x01}},
{0x4a,0x01,{0x00}},
{0x4b,0x01,{0x02}},
{0x4c,0x01,{0x07}},
{0x4d,0x01,{0x07}},
{0x4e,0x01,{0x07}},
{0x4f,0x01,{0x07}},
{0x50,0x01,{0x16}},  //CLK7
{0x51,0x01,{0x14}},  //CLK5
{0x52,0x01,{0x07}},
{0x53,0x01,{0x07}},
{0x54,0x01,{0x12}},  //CLK3
{0x55,0x01,{0x10}},  //CLK1
{0x56,0x01,{0x08}},  //STV_A
{0x57,0x01,{0x07}},
{0x58,0x01,{0x07}},
{0x59,0x01,{0x07}},
{0x5a,0x01,{0x07}},
{0x5b,0x01,{0x07}},
{0x5c,0x01,{0x07}},

{0x61,0x01,{0x08}},
{0x62,0x01,{0x07}},
{0x63,0x01,{0x01}},
{0x64,0x01,{0x00}},
{0x65,0x01,{0x02}},
{0x66,0x01,{0x07}},
{0x67,0x01,{0x07}},
{0x68,0x01,{0x07}},
{0x69,0x01,{0x07}},
{0x6a,0x01,{0x10}},
{0x6b,0x01,{0x12}},
{0x6c,0x01,{0x07}},
{0x6d,0x01,{0x07}},
{0x6e,0x01,{0x14}},
{0x6f,0x01,{0x16}},
{0x70,0x01,{0x20}},
{0x71,0x01,{0x07}},
{0x72,0x01,{0x07}},
{0x73,0x01,{0x07}},
{0x74,0x01,{0x07}},
{0x75,0x01,{0x07}},
{0x76,0x01,{0x07}},

{0x77,0x01,{0x09}},
{0x78,0x01,{0x07}},
{0x79,0x01,{0x01}},
{0x7a,0x01,{0x00}},
{0x7b,0x01,{0x02}},
{0x7c,0x01,{0x07}},
{0x7d,0x01,{0x07}},
{0x7e,0x01,{0x07}},
{0x7f,0x01,{0x07}},
{0x80,0x01,{0x11}},
{0x81,0x01,{0x13}},
{0x82,0x01,{0x07}},
{0x83,0x01,{0x07}},
{0x84,0x01,{0x15}},
{0x85,0x01,{0x17}},
{0x86,0x01,{0x21}},
{0x87,0x01,{0x07}},
{0x88,0x01,{0x07}},
{0x89,0x01,{0x07}},
{0x8a,0x01,{0x07}},
{0x8b,0x01,{0x07}},
{0x8c,0x01,{0x07}},

{0xB0,0x01,{0x12}},
{0xB1,0x01,{0x33}},
{0xB2,0x01,{0x22}},

{0xb9,0x01,{0x40}},

//0xCD,0x01,0x00    //µ÷ÕûCLK8¸úTSHDµÄÊ±¼äÊ±É¾µô  0702

{0xd0,0x01,{0x01}},
{0xd1,0x01,{0x00}},
{0xdc,0x01,{0x35}},
{0xdd,0x01,{0x42}},
{0xe2,0x01,{0x00}},
{0xe6,0x01,{0x22}},
{0xe7,0x01,{0x54}},  //V-porch SRC=V0

{0xFF,0x03,{0x98,0x81,0x02}},
{0x40,0x01,{0x4A}},
{0x4B,0x01,{0x5A}},
{0x4D,0x01,{0x4E}},
{0x4E,0x01,{0x00}},
{0x1A,0x01,{0x48}},

// GVDDP GVDDN VCOM VGH VGHO VGL VGLO setup
{0xFF,0x03,{0x98,0x81,0x05}},
{0x03,0x01,{0x00}},  //VCOM
{0x04,0x01,{0xCF}},  //VCOM
{0x58,0x01,{0x62}},  //VGL x2
{0x63,0x01,{0x7E}},  //GVDDN   -5.0V
{0x64,0x01,{0x7E}},  //GVDDP   5.0V
{0x68,0x01,{0xA1}},  //VGHO   15V
{0x69,0x01,{0xA7}},  //VGH    16V
{0x6A,0x01,{0xA1}},  //VGLO   -12V
{0x6B,0x01,{0x93}},  //VGL    -13V

// Resolution 720RGB*1600
{0xFF,0x03,{0x98,0x81,0x06}},
{0x2E,0x01,{0x01}}, //NL enable
{0xC0,0x01,{0x1F}},
{0xC1,0x01,{0x03}},

{0xFF,0x03,{0x98,0x81,0x06}},
{0x11,0x01,{0x03}},
{0x13,0x01,{0x44}},
{0x14,0x01,{0x41}},
{0x15,0x01,{0xF1}},
{0x16,0x01,{0x40}},
{0xC2,0x01,{0x04}},
{0x27,0x01,{0x20}},
{0x28,0x01,{0x20}},
{0x48,0x01,{0x0F}},
{0x4D,0x01,{0x80}},
{0x4E,0x01,{0x40}},
{0x7F,0x01,{0x78}},
{0xD6,0x01,{0x87}}, //FTE=TSVD1,0x FTE1=TSHD  85
{0xDD,0x01,{0x18}}, //3LANES

///////////////////GAMMA/////////////
{0xFF,0x03,{0x98,0x81,0x08}},
{0xE0,0x27,{0x00,0x24,0x72,0xA6,0xE5,0x55,0x17,0x3F,0x6B,0x8F,0xA5,0xC6,0xF1,0x18,0x3E,0xAA,0x67,0x99,0xBA,0xE4,0xFF,0x08,0x36,0x6E,0x9C,0x03,0xEC}},
{0xE1,0x27,{0x00,0x24,0x72,0xA6,0xE5,0x55,0x17,0x3F,0x6B,0x8F,0xA5,0xC6,0xF1,0x18,0x3E,0xAA,0x67,0x99,0xBA,0xE4,0xFF,0x08,0x36,0x6E,0x9C,0x03,0xEC}},

{0xFF,0x03,{0x98,0x81,0x0E}},
{0x00,0x01,{0xA0}}, //LV mode
{0x01,0x01,{0x26}},
{0x11,0x01,{0x90}},
{0x13,0x01,{0x10}},

{0xFF, 0x03, {0x98, 0x81, 0x03}},
{0x83, 0x01, {0x30}},
{0x84, 0x01, {0x00}},

{0xFF, 0x03, {0x98, 0x81, 0x00}},//Page0
{0x51, 0x02, {0x00, 0x00} },
{0x53, 0x01, {0x24}},
{0x55, 0x01, {0x00}},

{0x11,0x01,{0x00}},
{REGFLAG_DELAY,120,{}},
{0x29,0x01,{0x00}}, 
{REGFLAG_DELAY,100,{}},
{0x35,0x01,{0x00}},//TE enable
{REGFLAG_END_OF_TABLE, 0x00, {} }
};
#else
static struct LCM_setting_table init_setting[] = {
	{0xFF, 0x03, {0x98, 0x81, 0x03}},
	{0x83, 0x01, {0x30}},
	{0x84, 0x01, {0x00}},
	{0xFF, 0x03, {0x98, 0x81, 0x00}},//Page0
	{0x51, 0x02, {0x00, 0x00}},
	{0x53, 0x01, {0x24}},
	{0x55, 0x01, {0x00}},
	{0x11, 0x01, {0x00}},
	{REGFLAG_DELAY, 60,{}},
	{0x29, 0x01, {0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {} }

};
#endif
static struct LCM_setting_table bl_level[] = {
	{0x51, 0x02, {0x3F, 0xFF} },
	{REGFLAG_DELAY, 1, {} },
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
				LCM_LOGI("cmd=%x, %x\n",cmd,table[i].para_list[0]);
				dsi_set_cmdq_V22(cmdq, cmd, table[i].count,
						table[i].para_list, force_update);
		}
	}
}


static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	LCM_LOGI("%s: ili9881h\n",__func__);
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
	params->dsi.vertical_frontporch = 240; //old is 24,now is 124
	params->dsi.vertical_active_line = FRAME_HEIGHT;
	params->dsi.horizontal_sync_active = 20; //old is 20,now is 8
	params->dsi.horizontal_backporch = 20;//old is 60,now is 12
	params->dsi.horizontal_frontporch = 60;//old is 60,now is 16
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.PLL_CLOCK = 293;    /* FrameRate = 60Hz */ /* this value must be in MTK suggested table */

	params->dsi.ssc_disable = 0;
	params->dsi.ssc_range = 3;

#if 0
	params->dsi.HS_TRAIL = 7;
	params->dsi.HS_ZERO = 12;
	params->dsi.CLK_HS_PRPR = 6;
	params->dsi.LPX = 5;
	params->dsi.TA_GET = 25;
	params->dsi.TA_SURE = 7;
	params->dsi.TA_GO = 20;
	params->dsi.CLK_TRAIL = 9;
	params->dsi.CLK_ZERO = 33;
	params->dsi.CLK_HS_PRPR = 6;
	params->dsi.CLK_HS_POST = 36;
	params->dsi.CLK_HS_EXIT=10;
	params->dsi.CLK_TRAIL=9;
#endif

	//params->dsi.clk_lp_per_line_enable = 0;
	//params->dsi.esd_check_enable = 0;
	//params->dsi.customization_esd_check_enable = 1;
	params->dsi.cont_clock = 0;
	params->dsi.clk_lp_per_line_enable = 1;
	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd = 0x09;
	params->dsi.lcm_esd_check_table[0].count = 3;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x80;
	params->dsi.lcm_esd_check_table[0].para_list[1] = 0x03;
	params->dsi.lcm_esd_check_table[0].para_list[2] = 0x06;
	params->dsi.lcm_esd_check_table[1].cmd = 0x54;
	params->dsi.lcm_esd_check_table[1].count = 1;
	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x2c;
}

extern volatile int gesture_dubbleclick_en;
static void lcm_reset(void)
{
	if (!gesture_dubbleclick_en) {
		MDELAY(2);//t3
		SET_RESET_PIN(1);
		MDELAY(2);//t4
		tpd_gpio_output(0, 1);
	} else {
		SET_RESET_PIN(0);
		MDELAY(1);//t3
		SET_RESET_PIN(1);
		MDELAY(10);//t3
		tpd_gpio_output(0, 1);
	}

	LCM_LOGI("ili9881h lcm reset end.\n");
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
	unsigned char data = 0x0F;  //up to +/-5.5V
	int ret = 0;
	LCM_LOGI("%s: ili9881h start init\n",__func__);

	if (!gesture_dubbleclick_en) {
		SET_RESET_PIN(0);
		tpd_gpio_output(0, 0);

		set_gpio_lcd_enp(1);
		MDELAY(2);//t2
		set_gpio_lcd_enn(1);

		ret = NT50358A_write_byte(cmd, data);
		if (ret < 0)
			LCM_LOGI("----cmd=%0x--i2c write error----\n", cmd);
		else
			LCM_LOGI("---cmd=%0x--i2c write success----\n", cmd);
		cmd = 0x01;
		data = 0x0F;
		ret = NT50358A_write_byte(cmd, data);
		if (ret < 0)
			LCM_LOGI("---cmd=%0x--i2c write error----\n", cmd);
		else
			LCM_LOGI("----cmd=%0x--i2c write success----\n", cmd);
	}
	lcm_reset();

	push_table(NULL, init_setting,
			sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);

	LCM_LOGI("%s: ili9881h  init done\n",__func__);
}

static void lcm_suspend(void)
{
#ifndef MACH_FPGA

	LCM_LOGI("%s,ili9881h lcm_suspend start\n", __func__);
	push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
	if (!gesture_dubbleclick_en) {
		set_gpio_lcd_enn(0);
		MDELAY(2);//t11
		set_gpio_lcd_enp(0);
	}
#endif
	LCM_LOGI("%s,ili9881h lcm_suspend done\n", __func__);

}

static void lcm_resume(void)
{
	LCM_LOGI("%s ili9881h start",__func__);
	lcm_init();
	LCM_LOGI("%s ili9881h done\n",__func__);
}


static unsigned int lcm_ata_check(unsigned char *buffer)
{
	return 0;
}

static void lcm_setbacklight(void *handle, unsigned int level)
{
	if (level > 255)
		level = 255;
#if 0
	if (level < 2 && level !=0)
		level = 2;
#endif
	bl_level[0].para_list[0] = (level & 0xF0)>>4;
	bl_level[0].para_list[1] = (level & 0x0F)<<4;
	LCM_LOGI("%s, backlight set level = %d \n", __func__, level);
	push_table(handle, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}

static unsigned int lcm_compare_id(void)
{
#if 0
	return 1;
#else
	unsigned int id_reg_DC = 0,id_reg_DB = 255,id_reg_DA = 0;
	unsigned char buffer[2];
	unsigned int array[16];

	SET_RESET_PIN(0);
	MDELAY(5);

	SET_RESET_PIN(1);
	MDELAY(20);

	array[0] = 0x00023700;  /* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xDC, buffer, 1);
	id_reg_DC = buffer[0];         /* we only need ID */

	read_reg_v2(0xDB, buffer, 1);
	id_reg_DB = buffer[0];
	
	read_reg_v2(0xDA, buffer, 1);
	id_reg_DA = buffer[0];

	LCM_LOGI("%s,ili9881h vendor id_reg_DC=0x%08x, id_reg_DB=%08x, id_reg_DA=%08x\n", __func__, id_reg_DC, id_reg_DB, id_reg_DA);

	if ((LCM_ID_REG_DC == id_reg_DC) && (LCM_ID_REG_DC == id_reg_DB) && (LCM_ID_REG_DC == id_reg_DA))
		return 1;
	else
		return 0;
#endif
}


struct LCM_DRIVER ontim_ili9881h_hdplus_dsi_vdo_hlt_lcm_drv = {
	.name = "ontim_ili9881h_hdplus_dsi_vdo_hlt",
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
