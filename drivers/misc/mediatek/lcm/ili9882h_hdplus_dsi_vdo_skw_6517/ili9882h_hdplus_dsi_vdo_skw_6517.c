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
#define LOG_TAG "LCM-ili9882h skw"

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
#define LCM_ID_REG_DC (unsigned int)(0x64)
#define LCM_ID_REG_DB (unsigned int)(0x80)
#define LCM_ID_REG_DA (unsigned int)(0x00)
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
extern void ili_resume_by_ddi(void);
#endif
//static struct LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

static unsigned int is_bl_delay;

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

static struct LCM_setting_table init_setting[] = {
	{0xFF, 3, {0x98, 0x82, 0x01}},
	{0x00, 1, {0x42}},  //STV A rise
	{0x01, 1, {0x33}},  //STV A overlap
	{0x02, 1, {0x01}},
	{0x03, 1, {0x00}},
	{0x04, 1, {0x02}},  //STV B rise
	{0x05, 1, {0x31}},  //STV B overlap
	{0x07, 1, {0x00}},
	{0x08, 1, {0x80}},  //CLK A rise
	{0x09, 1, {0x81}},  //CLK A fall
	{0x0a, 1, {0x71}},  //CLK A overlap
	{0x0b, 1, {0x00}},
	{0x0c, 1, {0x01}},
	{0x0d, 1, {0x01}},
	{0x0e, 1, {0x00}},
	{0x0f, 1, {0x00}},
	{0x28, 1, {0x88}},
	{0x29, 1, {0x88}},
	{0x2a, 1, {0x00}},
	{0x2b, 1, {0x00}},

	{0x31, 1, {0x07}},
	{0x32, 1, {0x0c}},
	{0x33, 1, {0x22}},
	{0x34, 1, {0x23}}, //GOUT_4R_FW
	{0x35, 1, {0x07}},
	{0x36, 1, {0x08}},
	{0x37, 1, {0x16}},
	{0x38, 1, {0x14}},
	{0x39, 1, {0x12}},
	{0x3a, 1, {0x10}},
	{0x3b, 1, {0x21}},
	{0x3c, 1, {0x07}}, //GOUT_12R_FW
	{0x3d, 1, {0x07}},
	{0x3e, 1, {0x07}},
	{0x3f, 1, {0x07}},
	{0x40, 1, {0x07}},
	{0x41, 1, {0x07}},
	{0x42, 1, {0x07}},
	{0x43, 1, {0x07}},
	{0x44, 1, {0x07}},
	{0x45, 1, {0x07}},
	{0x46, 1, {0x07}},
	{0x47, 1, {0x07}},
	{0x48, 1, {0x0d}},
	{0x49, 1, {0x22}},
	{0x4a, 1, {0x23}},//GOUT_4L_FW
	{0x4b, 1, {0x07}},
	{0x4c, 1, {0x09}},
	{0x4d, 1, {0x17}},
	{0x4e, 1, {0x15}},
	{0x4f, 1, {0x13}},
	{0x50, 1, {0x11}},
	{0x51, 1, {0x21}},
	{0x52, 1, {0x07}},//GOUT_12L_FW
	{0x53, 1, {0x07}},
	{0x54, 1, {0x07}},
	{0x55, 1, {0x07}},
	{0x56, 1, {0x07}},
	{0x57, 1, {0x07}},
	{0x58, 1, {0x07}},
	{0x59, 1, {0x07}},
	{0x5a, 1, {0x07}},
	{0x5b, 1, {0x07}},
	{0x5c, 1, {0x07}},
	{0x61, 1, {0x07}},
	{0x62, 1, {0x0d}},
	{0x63, 1, {0x22}},
	{0x64, 1, {0x23}},//GOUT_4R_BW
	{0x65, 1, {0x07}},
	{0x66, 1, {0x09}},
	{0x67, 1, {0x11}},
	{0x68, 1, {0x13}},
	{0x69, 1, {0x15}},
	{0x6a, 1, {0x17}},
	{0x6b, 1, {0x21}},
	{0x6c, 1, {0x07}},//GOUT_12R_BW
	{0x6d, 1, {0x07}},
	{0x6e, 1, {0x07}},
	{0x6f, 1, {0x07}},
	{0x70, 1, {0x07}},
	{0x71, 1, {0x07}},
	{0x72, 1, {0x07}},
	{0x73, 1, {0x07}},
	{0x74, 1, {0x07}},
	{0x75, 1, {0x07}},
	{0x76, 1, {0x07}},
	{0x77, 1, {0x07}},
	{0x78, 1, {0x0c}},
	{0x79, 1, {0x22}},
	{0x7a, 1, {0x23}}, //GOUT_4L_BW
	{0x7b, 1, {0x07}},
	{0x7c, 1, {0x08}},
	{0x7d, 1, {0x10}},
	{0x7e, 1, {0x12}},
	{0x7f, 1, {0x14}},
	{0x80, 1, {0x16}},
	{0x81, 1, {0x21}},
	{0x82, 1, {0x07}}, //GOUT_12L_BW
	{0x83, 1, {0x07}},
	{0x84, 1, {0x07}},
	{0x85, 1, {0x07}},
	{0x86, 1, {0x07}},
	{0x87, 1, {0x07}},
	{0x88, 1, {0x07}},
	{0x89, 1, {0x07}},
	{0x8a, 1, {0x07}},
	{0x8b, 1, {0x07}},
	{0x8c, 1, {0x07}},
	{0xb0, 1, {0x33}},
	{0xba, 1, {0x06}},
	{0xc0, 1, {0x07}},
	{0xc1, 1, {0x00}},
	{0xc2, 1, {0x00}},
	{0xc3, 1, {0x00}},
	{0xc4, 1, {0x00}},
	{0xca, 1, {0x44}},
	{0xd0, 1, {0x01}},
	{0xd1, 1, {0x22}},
	{0xd3, 1, {0x40}},
	{0xd5, 1, {0x51}},
	{0xd6, 1, {0x20}},
	{0xd7, 1, {0x01}},
	{0xd8, 1, {0x00}},
	{0xdc, 1, {0xc2}},
	{0xdd, 1, {0x10}},
	{0xdf, 1, {0xb6}},
	{0xe0, 1, {0x3E}},
	{0xe2, 1, {0x47}}, //SRC power off GND
	{0xe7, 1, {0x54}},
	{0xe6, 1, {0x22}},
	{0xee, 1, {0x15}},
	{0xf1, 1, {0x00}},
	{0xf2, 1, {0x00}},
	{0xfa, 1, {0xdf}},

	// RTN. Internal VBP, Internal VFP
	{0xFF, 3, {0x98, 0x82, 0x02}},
	{0xF1, 1, {0x1C}},     // Tcon ESD option
	{0x40, 1, {0x4B}},     // Data_in=7us
	{0x4B, 1, {0x5A}},     // line_chopper
	{0x50, 1, {0xCA}},     // line_chopper
	{0x51, 1, {0x50}},     // line_chopper
	{0x06, 1, {0x8D}},     // Internal Line Time (RTN)
	{0x0B, 1, {0xA0}},     // Internal VFP[9]
	{0x0C, 1, {0x80}},     // Internal VFP[8]
	{0x0D, 1, {0x1A}},     // Internal VBP
	{0x0E, 1, {0x04}},     // Internal VFP
	{0x4D, 1, {0xCE}},     // Power Saving Off
	{0x73, 1, {0x04}},     // notch tuning
	{0x70, 1, {0x32}},     // notch tuning

	// Power Setting
	{0xFF, 3, {0x98, 0x82, 0x05}},
	{0xA8, 1, {0x62}},
	{0x03, 1, {0x00}},    // VCOM
	{0x04, 1, {0xAF}},    // VCOM
	{0x63, 1, {0x73}},    // GVDDN = -4.8V
	{0x64, 1, {0x73}},    // GVDDP = 4.8V
	{0x68, 1, {0x65}},    // VGHO = 12V
	{0x69, 1, {0x6B}},    // VGH = 13V
	{0x6A, 1, {0xA1}},    // VGLO = -12V
	{0x6B, 1, {0x93}},    // VGL = -13V
	{0x00, 1, {0x01}},    // Panda Enable
	{0x46, 1, {0x00}},    // LVD HVREG option
	{0x45, 1, {0x01}},    // VGHO/DCHG1/DCHG2 DELAY OPTION Default
	{0x17, 1, {0x50}},    // LVD rise debounce
	{0x18, 1, {0x01}},    // Keep LVD state
	{0x85, 1, {0x37}},    // HW RESET option
	{0x86, 1, {0x0F}},    // FOR PANDA GIP DCHG1/2ON EN

	// Resolution
	{0xFF, 3, {0x98, 0x82, 0x06}},
	{0xD9, 1, {0x1F}},    // 4Lane
	{0xC0, 1, {0x40}},    // NL = 1600
	{0xC1, 1, {0x16}},    // NL = 1600
	{0x07, 1, {0xF7}},    // ABNORMAL RESET OPTION
	{0x06, 0x01, {0xA4}},
	//Gamma Register
	{0xFF,3, {0x98,0x82,0x08}},
	{0xE0,27,{0x40,0x24,0x97,0xD1,0x10,0x55,0x40,0x64,0x8D,0xAD,0xA9,0xDE,0x03,0x24,0x43,0xAA,0x63,0x8C,0xA8,0xCC,0xFE,0xEC,0x18,0x50,0x7D,0x03,0xEC}},
	{0xE1,27,{0x40,0x24,0x97,0xD1,0x10,0x55,0x40,0x64,0x8D,0xAD,0xA9,0xDE,0x03,0x24,0x43,0xAA,0x63,0x8C,0xA8,0xCC,0xFE,0xEC,0x18,0x50,0x7D,0x03,0xEC}},

	// OSC Auto Trim Setting
	{0xFF, 3, {0x98, 0x82, 0x0B}},
	{0x9A, 1, {0x44}},
	{0x9B, 1, {0x88}},
	{0x9C, 1, {0x03}},
	{0x9D, 1, {0x03}},
	{0x9E, 1, {0x71}},
	{0x9F, 1, {0x71}},
	{0xAB, 1, {0xE0}},     // AutoTrimType

	{0xFF, 3, {0x98, 0x82, 0x0E}},
	{0x02, 1, {0x09}},
	{0x11, 1, {0x50}},      // TSVD Rise Poisition
	{0x13, 1, {0x10}},     // TSHD Rise Poisition
	{0x00, 1, {0xA0}},     // LV mode

	{0xFF, 3, {0x98, 0x82, 0x03}},  //pwm 11bit 15.6k
	{0x83, 1, {0xE8}},
	{0x84, 1, {0x0F}},

	{0xFF, 3, {0x98, 0x82, 0x00}},
	{0x51, 2, {0x00, 0xFF}},
	{0x53, 0x01, {0x2C}},
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0x29, 0x01, {0x00}},
	{REGFLAG_DELAY, 20,{}},
	{0x35, 0x01, {0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};
static struct LCM_setting_table bl_level[] = {
	{0x51, 0x02, {0x00, 0xFF} },
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
	params->dsi.vertical_sync_active = 4; //old is 2,now is 4
	params->dsi.vertical_backporch = 16; //old is 8,now is 100
	params->dsi.vertical_frontporch = 240; //old is 24,now is 124
	params->dsi.vertical_active_line = FRAME_HEIGHT;
	params->dsi.horizontal_sync_active = 20; //old is 20,now is 8
	params->dsi.horizontal_backporch = 80;//old is 60,now is 12
	params->dsi.horizontal_frontporch = 80;//old is 60,now is 16
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.PLL_CLOCK = 310;    /* FrameRate = 60Hz */ /* this value must be in MTK suggested table */

	params->dsi.ssc_disable = 1;
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
		//tpd_gpio_output(0, 1);
		MDELAY(5);
		ili_resume_by_ddi();
		MDELAY(15);//Tcmd
	} else {
		SET_RESET_PIN(0);
		MDELAY(1);//t3
		SET_RESET_PIN(1);
		MDELAY(10);//t3
		//tpd_gpio_output(0, 1);
	}

	LCM_LOGI(" lcm reset end.\n");
}

static void i2c_send_data_lcd(unsigned char cmd, unsigned char data)
{
	int ret = 0;

	ret = NT50358A_write_byte(cmd, data);
	if (ret < 0)
		LCM_LOGI("----cmd=%0x--i2c write error----\n", cmd);
	else{
		LCM_LOGI("---cmd=%0x--i2c write success----\n", cmd);
	}
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
	unsigned char data = 0x14;
	LCM_LOGI("%s:  start init\n",__func__);

	if (!gesture_dubbleclick_en) {
		SET_RESET_PIN(0);
		//tpd_gpio_output(0, 0);

		set_gpio_lcd_enp(1);
		MDELAY(6);//for bias IC and tr2
		i2c_send_data_lcd(cmd,data);

		MDELAY(2);//t2

		set_gpio_lcd_enn(1);
		MDELAY(7);//for bias IC and tr2
		cmd = 0x01;
		data = 0x14;
		i2c_send_data_lcd(cmd,data);
	}
	lcm_reset();

	push_table(NULL, init_setting,sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);

	is_bl_delay = TRUE;
	LCM_LOGI("%s:   init done\n",__func__);
}

static void lcm_suspend(void)
{
#ifndef MACH_FPGA
	LCM_LOGI("%s, lcm_suspend start\n", __func__);
	push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
	if (!gesture_dubbleclick_en) {
		set_gpio_lcd_enn(0);
		MDELAY(2);//t11
		set_gpio_lcd_enp(0);
	}
#endif
	LCM_LOGI("%s, lcm_suspend done\n", __func__);

}

static void lcm_resume(void)
{
	LCM_LOGI("%s  start",__func__);
	lcm_init();
	LCM_LOGI("%s  done\n",__func__);
}


static unsigned int lcm_ata_check(unsigned char *buffer)
{
	return 0;
}

static void lcm_setbacklight(void *handle, unsigned int level)
{
	if (level == 256) {
		level = 255;
	}else {
		if (level > 256 )
		level = 255;

		level = (level * 8) / 10;
	}
#if 0
	if (level < 2 && level !=0)
		level = 2;
#endif
	if (TRUE == is_bl_delay)
	{
		is_bl_delay = FALSE;
		LCM_LOGI("delay 15ms to set backlight, is_bl_delay = %d\n",is_bl_delay);
		MDELAY(15);//t7
	}
	bl_level[0].para_list[1] = level;
	LCM_LOGI("%s, backlight set level = %d \n", __func__, bl_level[0].para_list[1]);
	push_table(handle, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}

static unsigned int lcm_compare_id(void)
{
#if 0
	return 1;
#else
	unsigned int id_reg_DC = 0,id_reg_DA = 0,id_reg_DB = 0;
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

	LCM_LOGI("%s, vendor id_reg_DC=0x%08x, id_reg_DA=%08x,id_reg_DB=%08x\n", __func__, id_reg_DC, id_reg_DA, id_reg_DB);

	if ((LCM_ID_REG_DC == id_reg_DC))
		return 1;
	else
		return 0;
#endif
}


struct LCM_DRIVER ili9882h_hdplus_dsi_vdo_skw_6517_lcm_drv = {
	.name = "ili9882h_hdplus_dsi_vdo_skw_6517",
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
