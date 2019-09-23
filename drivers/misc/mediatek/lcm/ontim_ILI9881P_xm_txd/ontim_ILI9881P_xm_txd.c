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
#define LOG_TAG "LCM-ILI9881P-xm-txd"

#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif
#include "tpd.h" 
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
/*#include <mach/mt_pm_ldo.h>*/
#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_gpio.h>
#else
#include <mt-plat/mtk_gpio.h>
#include <mt-plat/mt6763/include/mach/gpio_const.h>
//#include "mach/gpio_const.h" 
#endif
#endif
#ifdef CONFIG_MTK_LEGACY
#include <cust_gpio_usage.h>
#endif
#ifndef CONFIG_FPGA_EARLY_PORTING
#if defined(CONFIG_MTK_LEGACY)
#include <cust_i2c.h>
#endif
#endif

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif


static const unsigned int BL_MIN_LEVEL = 20;
static LCM_UTIL_FUNCS lcm_util;

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

extern int NT50358A_write_byte(unsigned char addr, unsigned char value);

#endif

/* static unsigned char lcd_id_pins_value = 0xFF; */
static const unsigned char LCD_MODULE_ID = 0x01;
#define LCM_DSI_CMD_MODE                                    0
#define FRAME_WIDTH                                     (720)
#define FRAME_HEIGHT                                    (1440)

#define LCM_PHYSICAL_WIDTH                  (61880)
#define LCM_PHYSICAL_HEIGHT                    (123770)
#define REGFLAG_DELAY       0xFFFC
#define REGFLAG_UDELAY  0xFFFB
#define REGFLAG_END_OF_TABLE    0xFFFD
#define REGFLAG_RESET_LOW   0xFFFE
#define REGFLAG_RESET_HIGH  0xFFFF

//static LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef FT8006U_DELAY
#define FT8006U_DELAY 4 
#endif

#ifndef FT8006U_DELAY_BACK
#define FT8006U_DELAY_BACK 6 
#endif

//static unsigned int ontim_resume = 0;
struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0xFF, 0x03, {0x98, 0x81, 0x00} },
	{0x28, 0, {} },
	{REGFLAG_DELAY, 120, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 100, {} },
};
#if 0
static struct LCM_setting_table init_setting[] = {

	//========== Page 0 relative ==========
	{ 0xFF, 0x03, {0x98, 0x81, 0x03} },
	{ 0x01, 0x01, {0x00} },
	{ 0x02, 0x01, {0x00} },
	{ 0x03, 0x01, {0x56} },
	{ 0x04, 0x01, {0x13} },
	{ 0x05, 0x01, {0x53} },
	{ 0x06, 0x01, {0x06} },
	{ 0x07, 0x01, {0x01} },
	{ 0x08, 0x01, {0x00} },
	{ 0x09, 0x01, {0x00} },
	{ 0x0a, 0x01, {0x03} },
	{ 0x0b, 0x01, {0x03} },
	{ 0x0c, 0x01, {0x0B} },
	{ 0x0d, 0x01, {0x00} },
	{ 0x0e, 0x01, {0x00} },
	{ 0x0f, 0x01, {0x00} },
	{ 0x10, 0x01, {0x00} },
	{ 0x11, 0x01, {0x00} },
	{ 0x12, 0x01, {0x00} },
	{ 0x13, 0x01, {0x00} },
	{ 0x14, 0x01, {0x00} },
	{ 0x15, 0x01, {0x08} },
	{ 0x16, 0x01, {0x08} },
	{ 0x17, 0x01, {0x00} },
	{ 0x18, 0x01, {0x08} },
	{ 0x19, 0x01, {0x00} },
	{ 0x1a, 0x01, {0x00} },
	{ 0x1b, 0x01, {0x00} },
	{ 0x1c, 0x01, {0x00} },
	{ 0x1d, 0x01, {0x00} },
	{ 0x1e, 0x01, {0xC4} },
	{ 0x1f, 0x01, {0x80} },
	{ 0x20, 0x01, {0x00} },
	{ 0x21, 0x01, {0x01} },
	{ 0x22, 0x01, {0x01} },
	{ 0x23, 0x01, {0x01} },
	{ 0x24, 0x01, {0x8A} },
	{ 0x25, 0x01, {0x8A} },
	{ 0x26, 0x01, {0x8A} },
	{ 0x27, 0x01, {0x8A} },
	{ 0x28, 0x01, {0x3B} },
	{ 0x29, 0x01, {0x11} },
	{ 0x2a, 0x01, {0x00} },
	{ 0x2b, 0x01, {0x00} },
	{ 0x2c, 0x01, {0x00} },
	{ 0x2d, 0x01, {0x00} },
	{ 0x2e, 0x01, {0x00} },
	{ 0x2f, 0x01, {0x00} },
	{ 0x30, 0x01, {0x00} },
	{ 0x31, 0x01, {0x00} },
	{ 0x32, 0x01, {0x00} },
	{ 0x33, 0x01, {0x00} },
	{ 0x34, 0x01, {0x03} },
	{ 0x35, 0x01, {0x00} },
	{ 0x36, 0x01, {0x00} },
	{ 0x37, 0x01, {0x00} },
	{ 0x38, 0x01, {0x3C} },
	{ 0x39, 0x01, {0x00} },
	{ 0x3a, 0x01, {0x00} },
	{ 0x3b, 0x01, {0x00} },
	{ 0x3c, 0x01, {0x00} },
	{ 0x3d, 0x01, {0x00} },
	{ 0x3e, 0x01, {0x00} },
	{ 0x3f, 0x01, {0x00} },
	{ 0x40, 0x01, {0x00} },
	{ 0x41, 0x01, {0x00} },
	{ 0x42, 0x01, {0x00} },
	{ 0x43, 0x01, {0x00} },
	{ 0x44, 0x01, {0x00} },
	//{REGFLAG_UDELAY, 100, {} },
	/* GIP_2 */
	{ 0x50, 0x01, {0x01} },
	{ 0x51, 0x01, {0x23} },
	{ 0x52, 0x01, {0x45} },
	{ 0x53, 0x01, {0x67} },
	{ 0x54, 0x01, {0x89} },
	{ 0x55, 0x01, {0xab} },
	{ 0x56, 0x01, {0x01} },
	{ 0x57, 0x01, {0x23} },
	{ 0x58, 0x01, {0x45} },
	{ 0x59, 0x01, {0x67} },
	{ 0x5a, 0x01, {0x89} },
	{ 0x5b, 0x01, {0xab} },
	{ 0x5c, 0x01, {0xcd} },
	{ 0x5d, 0x01, {0xef} },

	{ 0x5e, 0x01, {0x01} },
	{ 0x5F, 0x01, {0x01} },
	{ 0x60, 0x01, {0x08} },
	{ 0x61, 0x01, {0x00} },
	{ 0x62, 0x01, {0x02} },
	{ 0x63, 0x01, {0x0F} },
	{ 0x64, 0x01, {0x0E} },
	{ 0x65, 0x01, {0x0D} },
	{ 0x66, 0x01, {0x0C} },
	{ 0x67, 0x01, {0x06} },
	{ 0x68, 0x01, {0x02} },
	{ 0x69, 0x01, {0x02} },
	{ 0x6a, 0x01, {0x02} },
	{ 0x6b, 0x01, {0x02} },
	{ 0x6c, 0x01, {0x02} },
	{ 0x6d, 0x01, {0x02} },
	{ 0x6e, 0x01, {0x02} },
	{ 0x6f, 0x01, {0x02} },
	{ 0x70, 0x01, {0x02} },
	{ 0x71, 0x01, {0x02} },
	{ 0x72, 0x01, {0x00} },
	{ 0x73, 0x01, {0x02} },
	{ 0x74, 0x01, {0x02} },

	{ 0x75, 0x01, {0x01} },
	{ 0x76, 0x01, {0x06} },
	{ 0x77, 0x01, {0x00} },
	{ 0x78, 0x01, {0x02} },
	{ 0x79, 0x01, {0x0C} },
	{ 0x7a, 0x01, {0x0D} },
	{ 0x7b, 0x01, {0x0E} },
	{ 0x7c, 0x01, {0x0F} },
	{ 0x7d, 0x01, {0x08} },
	{ 0x7e, 0x01, {0x02} },
	{ 0x7f, 0x01, {0x02} },
	{ 0x80, 0x01, {0x02} },
	{ 0x81, 0x01, {0x02} },
	{ 0x82, 0x01, {0x02} },
	{ 0x83, 0x01, {0x02} },
	{ 0x84, 0x01, {0x02} },
	{ 0x85, 0x01, {0x02} },
	{ 0x86, 0x01, {0x02} },
	{ 0x87, 0x01, {0x02} },
	{ 0x88, 0x01, {0x00} },
	{ 0x89, 0x01, {0x02} },
	{ 0x8A, 0x01, {0x02} },
	//{REGFLAG_UDELAY, 100, {} },
	/* CMD_Page 4 */
	{ 0xFF, 0x03, {0x98, 0x81, 0x04} },
	{ 0x00, 0x01, {0x00} },
	{ 0x6C, 0x01, {0x15} },
	{ 0x6E, 0x01, {0x2A} },
	{ 0x6F, 0x01, {0x55} },
	{ 0x3A, 0x01, {0xA4} },
	{ 0x8D, 0x01, {0x1F} },
	{ 0x87, 0x01, {0xBA} },
	{ 0x26, 0x01, {0x76} },
	{ 0xB2, 0x01, {0xD1} },
	{ 0x88, 0x01, {0x0B} },
	{ 0x33, 0x01, {0x14} },
	{ 0x35, 0x01, {0x17} },
	{ 0x38, 0x01, {0x01} },
	{ 0x39, 0x01, {0x00} },
	{ 0x7A, 0x01, {0x0F} },
	{ 0xB5, 0x01, {0x02} },
	{ 0x31, 0x01, {0x25} },
	{ 0x3B, 0x01, {0x98} },
	//{REGFLAG_UDELAY, 100, {} },
	/* CMD_Page 1 */
	{ 0xFF, 0x03, {0x98, 0x81, 0x01} },
	{ 0x22, 0x01, {0x09} },
	{ 0x2E, 0x01, {0xF0} },
	{ 0x31, 0x01, {0x00} },
	{ 0x53, 0x01, {0x5A} },
	{ 0x55, 0x01, {0x5A} },
	{ 0x50, 0x01, {0xBF} },
	{ 0x51, 0x01, {0xBA} },
	{ 0x60, 0x01, {0x10} },
	{ 0x61, 0x01, {0x00} },
	{ 0x62, 0x01, {0x20} },
	{ 0x63, 0x01, {0x00} },
	/* ============Gamma START============= */
	/* Pos Register */
	{ 0xA0, 0x01, {0x00} },
	{ 0xA1, 0x01, {0x1F} },
	{ 0xA2, 0x01, {0x2C} },
	{ 0xA3, 0x01, {0x18} },
	{ 0xA4, 0x01, {0x1D} },
	{ 0xA5, 0x01, {0x31} },
	{ 0xA6, 0x01, {0x25} },
	{ 0xA7, 0x01, {0x25} },
	{ 0xA8, 0x01, {0x96} },
	{ 0xA9, 0x01, {0x1C} },
	{ 0xAA, 0x01, {0x28} },
	{ 0xAB, 0x01, {0x79} },
	{ 0xAC, 0x01, {0x1A} },
	{ 0xAD, 0x01, {0x1D} },
	{ 0xAE, 0x01, {0x54} },
	{ 0xAF, 0x01, {0x27} },
	{ 0xB0, 0x01, {0x2D} },
	{ 0xB1, 0x01, {0x51} },
	{ 0xB2, 0x01, {0x5F} },
	{ 0xB3, 0x01, {0x3F} },
	/* Neg Register */
	{ 0xC0, 0x01, {0x00} },
	{ 0xC1, 0x01, {0x1F} },
	{ 0xC2, 0x01, {0x2B} },
	{ 0xC3, 0x01, {0x1B} },
	{ 0xC4, 0x01, {0x1C} },
	{ 0xC5, 0x01, {0x30} },
	{ 0xC6, 0x01, {0x26} },
	{ 0xC7, 0x01, {0x25} },
	{ 0xC8, 0x01, {0x97} },
	{ 0xC9, 0x01, {0x1A} },
	{ 0xCA, 0x01, {0x28} },
	{ 0xCB, 0x01, {0x79} },
	{ 0xCC, 0x01, {0x1C} },
	{ 0xCD, 0x01, {0x1D} },
	{ 0xCE, 0x01, {0x54} },
	{ 0xCF, 0x01, {0x27} },
	{ 0xD0, 0x01, {0x2D} },
	{ 0xD1, 0x01, {0x51} },
	{ 0xD2, 0x01, {0x5F} },
	{ 0xD3, 0x01, {0x3F} },
	/* ============ Gamma END=========== */
	/* CMD_Page 2 */
	{ 0xFF, 0x03, {0x98,0x81,0x02} },
	{ 0x03, 0x01, {0x4D} },
	{ 0x04, 0x01, {0x45} },
	{ 0x05, 0x01, {0x44} },
	{ 0x06, 0x01, {0x20} },
	{ 0x07, 0x01, {0x00} },//pwm 30k
	{ 0xFF, 0x03, {0x98, 0x81, 0x00} },
	{ 0x51, 0x02, {0x00, 0x00} },
	{ 0x53, 0x01, {0x2C} },
	{ 0x55, 0x01, {0x01} },
	{ 0x35, 0x01, {0x00} },
	{ 0x11, 0x01, {0x00} },
	{REGFLAG_DELAY, 65, {} },
	{ 0x29, 0x01, {0x00} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};
#else
static struct LCM_setting_table init_setting[] = {
	{ 0xFF, 0x03, {0x98,0x81,0x05} },
	{ 0xB2, 0x01, {0x70} },
	{ 0xFF, 0x03, {0x98,0x81,0x03} },
	{ 0x83, 0x01, {0x60} },
	{ 0x84, 0x01, {0x02} },//pwm 31k
	{ 0xFF, 0x03, {0x98, 0x81, 0x00} },
	{ 0x36, 0x01, {0x03} },
	{ 0x51, 0x02, {0x00, 0x00} },
	{ 0x53, 0x01, {0x2C} },
	{ 0x55, 0x01, {0x00} },
	{ 0x35, 0x01, {0x00} },
	{ 0x11, 0x01, {0x00} },
	{REGFLAG_DELAY, 65, {} },
	{ 0xFF, 0x03, {0x98, 0x81, 0x02} },
	{ 0x01, 0x01, {0x54} },
	{REGFLAG_DELAY, 20, {} },
	{ 0xFF, 0x03, {0x98, 0x81, 0x00} },
	{ 0x29, 0x01, {0x00} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
    
};
#endif

static struct LCM_setting_table bl_level[] = {
#if 0
	{0x51, 0x01, {0xFF} },
#else
	{0x51, 0x02, {0x0F,0xF0} },
#endif
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static void push_table(void *cmdq, struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
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
	LCM_LOGI("%s: \n",__func__);
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
	params->dsi.LANE_NUM = LCM_THREE_LANE;
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
	params->dsi.vertical_backporch = 20; //old is 8,now is 100
	params->dsi.vertical_frontporch = 12; //old is 15,now is 24
	params->dsi.vertical_active_line = FRAME_HEIGHT;
	params->dsi.horizontal_sync_active = 40;
	params->dsi.horizontal_backporch = 72;
	params->dsi.horizontal_frontporch = 72;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_disable = 0;
	params->dsi.ssc_range = 3;
	//params->dsi.HS_TRAIL = 15;
	params->dsi.PLL_CLOCK = 336;    /* this value must be in MTK suggested table */
	params->dsi.PLL_CK_CMD = 300;    
	params->dsi.PLL_CK_VDO = 300;     
	params->dsi.CLK_HS_POST = 36;

	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 0;
	
	params->dsi.lcm_esd_check_table[0].cmd = 0x09;
	params->dsi.lcm_esd_check_table[0].count = 3;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x81;
	params->dsi.lcm_esd_check_table[0].para_list[1] = 0x83;
	params->dsi.lcm_esd_check_table[0].para_list[2] = 0x06;
	params->dsi.lcm_esd_check_table[1].cmd = 0x54;
	params->dsi.lcm_esd_check_table[1].count = 1;
}

#ifdef BUILD_LK
static struct mt_i2c_t NT50358A_i2c;

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

//#define GPIO_LCD_RST_PIN         (GPIO83| 0x80000000)
//#define GPIO_LCD_BIAS_ENN_PIN         (GPIO112 | 0x80000000)
//#define GPIO_LCD_BIAS_ENP_PIN         (GPIO122 | 0x80000000)
#define CTP_RST_GPIO_PINCTRL_ONTIM 0
static void lcm_reset(void)
{
	//printf("[uboot]:lcm reset start.\n");
#if 0
	if(ontim_resume){
		ontim_resume = 0;
		//        if(tpd_gpio_output != NULL)
		MDELAY(1);
		tpd_gpio_output(CTP_RST_GPIO_PINCTRL_ONTIM,0);
		MDELAY(10);
		tpd_gpio_output(CTP_RST_GPIO_PINCTRL_ONTIM,1);
	}
#endif
#ifdef GPIO_LCD_RST_PIN
	mt_set_gpio_mode(GPIO_LCD_RST_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_RST_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ONE);
	MDELAY(1);
	mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ZERO);
	MDELAY(10);
	mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ONE);
	MDELAY(50);
#else
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(10);

#endif
	LCM_LOGI("lcm reset end.\n");
}

static void lcm_init_power(void)
{
	/*#ifndef MACH_FPGA
#ifdef BUILD_LK
mt6325_upmu_set_rg_vgp1_en(1);
#else
LCM_LOGI("%s, begin\n", __func__);
hwPowerOn(MT6325_POWER_LDO_VGP1, VOL_DEFAULT, "LCM_DRV");
LCM_LOGI("%s, end\n", __func__);
#endif
#endif*/
}

static void lcm_suspend_power(void)
{
	/*#ifndef MACH_FPGA
#ifdef BUILD_LK
mt6325_upmu_set_rg_vgp1_en(0);
#else
LCM_LOGI("%s, begin\n", __func__);
hwPowerDown(MT6325_POWER_LDO_VGP1, "LCM_DRV");
LCM_LOGI("%s, end\n", __func__);
#endif
#endif*/
}

static void lcm_resume_power(void)
{
	/*#ifndef MACH_FPGA
#ifdef BUILD_LK
mt6325_upmu_set_rg_vgp1_en(1);
#else
LCM_LOGI("%s, begin\n", __func__);
hwPowerOn(MT6325_POWER_LDO_VGP1, VOL_DEFAULT, "LCM_DRV");
LCM_LOGI("%s, end\n", __func__);
#endif
#endif*/
}

static void lcm_init(void)
{
	unsigned char cmd = 0x0;
	unsigned char data = 0x0F;  //up to +/-5.5V
	int ret = 0;
	LCM_LOGI("%s: \n",__func__);

	ret = NT50358A_write_byte(cmd, data);
	if (ret < 0)
		LCM_LOGI("----nt50358a----cmd=%0x--i2c write error----\n", cmd);
	else
		LCM_LOGI("----nt50358a----cmd=%0x--i2c write success----\n", cmd);
	cmd = 0x01;
	data = 0x0F;
	ret = NT50358A_write_byte(cmd, data);
	if (ret < 0)
		LCM_LOGI("----nt50358a----cmd=%0x--i2c write error----\n", cmd);
	else
		LCM_LOGI("----nt50358a----cmd=%0x--i2c write success----\n", cmd);

#if defined(GPIO_LCD_BIAS_ENN_PIN)||defined(GPIO_LCD_BIAS_ENP_PIN)
#ifdef GPIO_LCD_BIAS_ENP_PIN 
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
	//mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);
#endif    

#ifdef GPIO_LCD_BIAS_ENN_PIN 
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
	//mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);
#endif

#ifdef GPIO_LCD_BIAS_ENP_PIN 
	MDELAY(20);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
#endif    

#ifdef GPIO_LCD_BIAS_ENN_PIN 
	MDELAY(5);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);
#endif
#else
	set_gpio_lcd_enp(1);
	MDELAY(1);
	set_gpio_lcd_enn(1);
	MDELAY(1);

#endif
	lcm_reset();

	push_table(NULL, init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
#ifndef MACH_FPGA

	push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
	MDELAY(10);
#if defined(GPIO_LCD_BIAS_ENN_PIN)||defined(GPIO_LCD_BIAS_ENP_PIN)

#ifdef GPIO_LCD_BIAS_ENN_PIN 
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);
#endif

#ifdef GPIO_LCD_BIAS_ENP_PIN 
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);
#endif    
#else
	/*SET_RESET_PIN(0);
	MDELAY(2);*/
	//+ add by fxz
	set_gpio_lcd_enn(0);
	MDELAY(1);
	set_gpio_lcd_enp(0);
	//- add by fxz
#endif

#endif
	LCM_LOGI("%s,lcm_suspend\n", __func__);

}

static void lcm_resume(void)
{
	LCM_LOGI("%s:  start\n",__func__);
	//ontim_resume = 1;
	lcm_init();
	LCM_LOGI("%s:  done\n",__func__);
}


//static struct LCM_setting_table read_id[] = {
//    {0xFF, 3, {0x98,0x81,0x01} },
//    {REGFLAG_END_OF_TABLE, 0x00, {} }
//};



static unsigned int lcm_ata_check(unsigned char *buffer)
{
#ifndef BUILD_LK
	unsigned int ret = 0;
	return ret;
#else
	return 0;
#endif
}


static void lcm_setbacklight(void *handle, unsigned int level)
{ 
	LCM_LOGI("%s,set backlight: level = %d\n", __func__, level);
	if (level > 255)
		level = 255;
	if (level < 3 && level !=0)
		level = 3;
#if 0
	bl_level[0].para_list[0] = level;
#else
	bl_level[0].para_list[0] = (level & 0xF0)>>4;
	bl_level[0].para_list[1] = ((level & 0x0F)<<4);
	LCM_LOGI("%s,set backlight: level = 0x%x %x\n", __func__, bl_level[0].para_list[0], bl_level[0].para_list[1]);
#endif
	push_table(handle, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}



LCM_DRIVER ontim_ILI9881P_lcm_drv_xm_txd = {
	.name = "ontim_ILI9881P_xm_txd",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	//    .compare_id = lcm_compare_id,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.set_backlight_cmdq = lcm_setbacklight,
	.ata_check = lcm_ata_check,

};
