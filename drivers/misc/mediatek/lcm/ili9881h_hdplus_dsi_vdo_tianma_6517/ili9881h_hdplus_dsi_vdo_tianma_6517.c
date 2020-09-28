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
#define LOG_TAG "LCM-ili9881H TIANMA"

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
#define LCM_ID_REG_DC (unsigned int)(0x90)
#define LCM_ID_REG_DB (unsigned int)(0x00)
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
extern void ilitek_resume_by_ddi(void);
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
{0xFF, 0x03, {0x98, 0x81, 0x00}},//Page0
{0x11, 0x01, {0x00}},
{REGFLAG_DELAY, 80,{}},

//improve display 0196251
{0xFF, 0x03, {0x98,0x81,0x01}},
{0x24, 0x01, {0xCC}},
{0x25, 0x01, {0x0B}},
{0x35, 0x01, {0x21}},  //STV_A_2
{0x47, 0x01, {0x07}},  //VGLO
{0x4B, 0x01, {0x20}},  //STV_A_1
{0xB8, 0x01, {0x20}},
{0xB9, 0x01, {0x20}},

{0xFF, 0x03, {0x98,0x81,0x02}},
{0x01, 0x01, {0x35}}, //MIPI time out 20200713
{0x06, 0x01, {0x9F}},

{0xFF, 0x03, {0x98,0x81,0x03}},
{0x85, 0x01, {0x30}},
{0x87, 0x01, {0x40}},
{0x93, 0x01, {0xF7}},
{0x9E, 0x01, {0x0F}},
{0x9F, 0x01, {0x0F}},
{0xA0, 0x01, {0x0F}},
{0xA1, 0x01, {0x10}},
{0xA2, 0x01, {0x11}},
{0xA3, 0x01, {0x12}},
{0xA4, 0x01, {0x13}},
{0xA5, 0x01, {0x14}},
{0xA6, 0x01, {0x95}},
{0xA7, 0x01, {0x96}},

{0xFF, 0x03, {0x98,0x81,0x05}},
{0x5D, 0x01, {0x07}}, //20200819 VGLO short VGL
//{0x5D, 0x01, {0x0F}}, //20200713

{0x6A, 0x01, {0x5B}}, //VGLO
{0x6B, 0x01, {0x43}}, //4D  VGL   20200819

{0xCC, 0x01, {0x5B}}, //VGLO_C
{0xCD, 0x01, {0x5B}}, //VGLO_L
{0xCE, 0x01, {0x5B}}, //VGLO_M
{0xCF, 0x01, {0x5B}}, //VGLO_H
{0xD4, 0x01, {0x43}}, //VGL_C
{0xD5, 0x01, {0x43}}, //VGL_L
{0xD6, 0x01, {0x43}}, //VGL_M
{0xD7, 0x01, {0x43}}, //VGL_H
{0xE0, 0x01, {0x03}},  //temp sensor enable
{0xE1, 0x01, {0x07}},  //speed up   20200813

{0xFF, 0x03, {0x98,0x81,0x06}},
{0x13, 0x01, {0x29}}, //20  20200818
{0x14, 0x01, {0x41}}, //20  20200818
{0x15, 0x01, {0xD5}}, //20  20200818
{0x16, 0x01, {0x40}}, //20  20200818
{0x27, 0x01, {0x32}}, //20  20200804
{0x28, 0x01, {0x18}}, //20  20200804

{0xFF, 0x03, {0x98,0x81,0x0E}},
{0x05, 0x01, {0x00}}, //01  20200819
{0x45, 0x01, {0x0C}}, //0B  20200818
{0x46, 0x01, {0x7F}}, //D0  20200818
{0x4D, 0x01, {0x8B}}, //8D  20200818

{0xFF, 0x03, {0x98, 0x81, 0x00}},//Page0
{0x51, 0x02, {0x00, 0x00}},
{0x53, 0x01, {0x2C}},
{0x55, 0x01, {0x03}}, //UI mode:power save hight
{0x68, 0x02, {0x04,0x00}},

{0xFF, 0x03, {0x98,0x81,0x03}},
{0x80, 0x01, {0x04}}, // 1 frame, 16 step
{0x81, 0x01, {0x04}}, // 1 frame, 16 step
{0x82, 0x01, {0x04}}, // 1 frame, 16 step
{0x83, 0x01, {0xE8}},                // DBV[7:0]
{0x84, 0x01, {0x00}},
{0xBC, 0x01, {0x06}}, //pwm 15.6kHz

{0xFF, 0x03, {0x98, 0x81, 0x00}},//Page0
{0x29, 0x01, {0x00}},
{REGFLAG_DELAY, 20,{}},
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
	params->dsi.vertical_backporch = 24; //old is 8,now is 100
	params->dsi.vertical_frontporch = 44; //old is 24,now is 124
	params->dsi.vertical_active_line = FRAME_HEIGHT;
	params->dsi.horizontal_sync_active = 72; //old is 20,now is 8
	params->dsi.horizontal_backporch = 120;//old is 60,now is 12
	params->dsi.horizontal_frontporch = 120;//old is 60,now is 16
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.PLL_CLOCK = 329;    /* FrameRate = 60Hz */ /* this value must be in MTK suggested table */

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
		ilitek_resume_by_ddi();
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

		if (1 == level) {
			level = 1;
		} else {
			level = (level * 40) / 51;
		}
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
	unsigned int id_reg_DC = 0,id_reg_DA = 0;
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

	read_reg_v2(0xDA, buffer, 1);
	id_reg_DA = buffer[0];

	LCM_LOGI("%s, vendor id_reg_DC=0x%08x, id_reg_DA=%08x\n", __func__, id_reg_DC, id_reg_DA);

	if ((LCM_ID_REG_DC == id_reg_DC) && (LCM_ID_REG_DA == id_reg_DA))
		return 1;
	else
		return 0;
#endif
}


struct LCM_DRIVER ili9881h_hdplus_dsi_vdo_tianma_6517_lcm_drv = {
	.name = "ili9881h_hdplus_dsi_vdo_tianma_6517",
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
