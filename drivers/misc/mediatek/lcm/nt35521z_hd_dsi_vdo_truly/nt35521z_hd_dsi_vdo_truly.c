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
#define FRAME_HEIGHT                                    (1440)
#define LCM_DENSITY		(320)

#define LCM_PHYSICAL_WIDTH                  (61880)
#define LCM_PHYSICAL_HEIGHT                    (123770)
#define REGFLAG_DELAY       0xFFFC
#define REGFLAG_UDELAY  0xFFFB
#define REGFLAG_END_OF_TABLE    0xFFFD
#define REGFLAG_RESET_LOW   0xFFFE
#define REGFLAG_RESET_HIGH  0xFFFF
extern int NT50358A_write_byte(unsigned char addr, unsigned char value);
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
    {0x28,1, {0x00} },
    {REGFLAG_DELAY, 1, {} },
    {0x10, 1, {0x00} },
    {REGFLAG_DELAY, 100, {} },
//    {0x04, 1, {0x5a} },
//    {REGFLAG_DELAY, 5, {} },
//    {0x05, 1, {0x5a} },
//    {REGFLAG_DELAY, 20, {} },
};

static struct LCM_setting_table init_setting[] = {

		//========== Page 0 relative ==========
		{0xF0,0x5,{0x55,0xAA,0x52,0x08,0x00}},
		// ##720RGB
		{0xB1,0x2,{0x68,0x21}},
		// ##3lanes
		{0xB2,0x3,{0x65,0x02,0x40}},
		// ##NL, (1440-480)/4=240=F0h
		{0xB5,0x2,{0xF0,0x00}},
		//SOURCE EQ
		{0xB8,0x2,{0x00,0x00}},
		//SOURCE DRIVER CONTROL
		{0xBB,0x2,{0x24,0x20}},
		// ##RTN, T1A, VBP, VFP
		{0xBD,0x5,{0x90,0x9C,0x10,0x14,0x01}},
		// ##Watch dog
		{0xC8,0x1,{0x80}},
		//for dimming
		{0xD3,0x1,{0x33}},
		{0xD6,0x2,{0x04,0x00}},
		{0xD9,0x3,{0x02,0x03,0xFD}},
		{0xE7,0x0A,{0xFF,0xF7,0xEA,0xDD,0xD0,0xC4,0xB7,0xB7,0xB7,0xB2}},


		//========== Page 1 relative ==========
		{0xF0,0x05,{0x55,0xAA,0x52,0x08,0x01}},
		// ##VGH=15V
		{0xB3,0x01,{0x26}},
		// ##VGL=-12V
		{0xB4,0x01,{0x19}},
		// ##VGH & VGL control
		{0xBB,0x01,{0x05}},
		// ##VGMP=5V
		{0xBC,0x01,{0x58}},//4.1
		// ##VGMN=-5V
		{0xBD,0x01,{0x58}},//4.1
		// ##VCOM=-1V
		//{0x29, 0, 0, 0, 0, 2,{ 0xBE,0x2B}},

		//========== Page 2 relative ==========
		{0xF0,0x05,{0x55,0xAA,0x52,0x08,0x02}},
		{0xED,0x01,{0x03}},
		{0xE9,0x0A,{0x56,0xF7,0x31,0x44,0x00,0x56,0xF7,0x31,0x44,0x00}},
		{0xEA,0x0A,{0x56,0xF7,0x31,0x44,0x00,0x56,0xF7,0x31,0x44,0x00}},
		{0xEB,0x0A,{0x56,0xF7,0x31,0x44,0x00,0x56,0xF7,0x31,0x44,0x00}},
		//Gamma Setting
		{0xEE,0x01,{0x01}},
		//R(+) MCR cmd
		{0xB0,0x10,{0x00,0x00,0x00,0x36,0x00,0x6C,0x00,0x8E,0x00,0xA8,0x00,0xBE,0x00,0xD0,0x00,0xE1}},
		{0xB1,0x10,{0x00,0xF0,0x01,0x24,0x01,0x4A,0x01,0x8B,0x01,0xBB,0x02,0x07,0x02,0x43,0x02,0x45}},
		{0xB2,0x10,{0x02,0x7D,0x02,0xB7,0x02,0xDD,0x03,0x13,0x03,0x31,0x03,0x60,0x03,0x6F,0x03,0x7F}},
		{0xB3,0x0B,{0x03,0x92,0x03,0xA7,0x03,0xBC,0x03,0xCE,0x03,0xD8,0x03,0xFF}},

		//========== Page 3 relative ==========
		{0xF0,0x05,{0x55,0xAA,0x52,0x08,0x03}},
		// RISING EQ CONROL
		{0xB0,0x02,{0x08,0x0C}},
		// ##STV01
		{0xB2,0x05,{0x00,0x0D,0x07,0x36,0x36}},
		// ##RST01
		{0xB6,0x05,{0x00,0xA7,0xAC,0x08,0x08}},
		// ##CLK01
		{0xBA,0x04,{0x66,0x00,0x40,0x02}},
		// ##CLK02
		{0xBB,0x04,{0x66,0x00,0x40,0x02}},
		{0xD0,0x01,{0x00}},

		//========== Page 4 relative ==========
		{0xF0,0x05,{0x55,0xAA,0x52,0x08,0x04}},
		// ##GPIO0=LEDPWM, GPIO1=TE
		{0xB1,0x02,{0x03,0x02}},
		{0xC7,0x04,{0xA0,0x0A,0x14,0x03}},
		{0xD3,0x01,{0x01}},

		//========== Page 5 relative ==========
		{0xF0,0x05,{0x55,0xAA,0x52,0x08,0x05}},
		// ##STV ON_Seq
		{0xB0,0x01,{0x06}},
		// ##RST ON_Seq
		{0xB1,0x01,{0x06}},
		// ##CLK ON_Seq
		{0xB2,0x02,{0x06,0x00}},
		// ##VDC01 ON_Seq,
		{0xB3,0x05,{0x0E,0x00,0x00,0x00,0x00}},
		// ##VDC02 ON_Seq,
		{0xB4,0x05,{0x06,0x00,0x00,0x00,0x00}},
		// ##IP Enable
		{0xBD,0x05,{0x03,0x01,0x01,0x00,0x03}},
		// ##CLK01
		{0xC0,0x02,{0x0B,0x30}},
		// ##CLK02
		{0xC1,0x02,{0x05,0x30}},
		// ##CLK01, Proch
		{0xD1,0x05,{0x00,0x05,0xAB,0x00,0x00}},
		// ##CLK02, Proch
		{0xD2,0x05,{0x00,0x05,0xB1,0x00,0x00}},
		// ##Abnormal, 2 frame
		{0xE3,0x01,{0x84}},
		// ##CLK
		{0xE5,0x01,{0x1A}},
		// ##STV
		{0xE6,0x01,{0x1A}},
		// ##RST
		{0xE7,0x01,{0x1A}},
		// ##VDC01
		{0xE9,0x01,{0x1A}},
		// ##VDC02
		{0xEA,0x01,{0x1A}},

		//========== Page 6 relative ==========
		{0xF0,0x05,{0x55,0xAA,0x52,0x08,0x06}},
		{0xB0,0x05,{0x08,0x31,0x30,0x10,0x18}},
		{0xB1,0x05,{0x12,0x1A,0x14,0x1C,0x31}},
		{0xB2,0x05,{0x31,0x35,0x35,0x00,0x35}},
		{0xB3,0x05,{0x35,0x35,0x35,0x35,0x35}},
		{0xB4,0x05,{0x35,0x35,0x35,0x35,0x35}},
		{0xB5,0x05,{0x35,0x01,0x35,0x35,0x31}},
		{0xB6,0x05,{0x31,0x1D,0x15,0x1B,0x13}},
		{0xB7,0x05,{0x19,0x11,0x30,0x31,0x09}},
		{0xC0,0x05,{0x01,0x30,0x31,0x1D,0x15}},
		{0xC1,0x05,{0x1B,0x13,0x19,0x11,0x31}},
		{0xC2,0x05,{0x31,0x35,0x35,0x09,0x35}},
		{0xC3,0x05,{0x35,0x35,0x35,0x35,0x35}},
		{0xC4,0x05,{0x35,0x35,0x35,0x35,0x35}},
		{0xC5,0x05,{0x35,0x08,0x35,0x35,0x31}},
		{0xC6,0x05,{0x31,0x10,0x18,0x12,0x1A}},
		{0xC7,0x05,{0x14,0x1C,0x31,0x30,0x00}},

		{0xD1,0x04,{0x35,0x35,0x35,0x35}},
		{0xD2,0x04,{0x35,0x35,0x35,0x35}},

		//========== CMD 2 Disable ==========
		{0xF0,0x05,{0x55,0xAA,0x52,0x00,0x00}},

		//## TE output
		{0x35,0x01,{0x00}},
		//CABC
		{0x51,0x01,{0x00}},
		{0x53,0x01,{0x24}},
		{0x55,0x01,{0x00}},
		{0x11,0,{}},
		{REGFLAG_DELAY, 120, {} },
		{0x29,0,{}},
		{REGFLAG_DELAY, 1, {} },

		//============ BIST Mode ==============
		//{0x29, 0, 0, 0, 0, 6,{ 0xF0,0x55,0xAA,0x52,0x08,0x00}},

		 //{0x29, 0, 0, 0, 0, 5,{ 0xEE,0x87,0x78,0x02,0x40}},
		{REGFLAG_END_OF_TABLE, 0x00, {} }

};
static struct LCM_setting_table bl_level[] = {
    {0x51, 0x01, {0xFF} },
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
		}
	}
}


static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    LCM_LOGI("%s: truly_NT35521Z\n",__func__);
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
    params->density = LCM_DENSITY;


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
    params->dsi.vertical_backporch = 14; //old is 8,now is 100
    params->dsi.vertical_frontporch = 32; //old is 15,now is 24
    params->dsi.vertical_active_line = FRAME_HEIGHT;
    params->dsi.horizontal_sync_active = 40;
    params->dsi.horizontal_backporch = 62;
    params->dsi.horizontal_frontporch = 62;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;
    params->dsi.ssc_disable = 0;
    params->dsi.ssc_range = 3;
    //params->dsi.HS_TRAIL = 15;
    params->dsi.PLL_CLOCK = 335;    /* this value must be in MTK suggested table */
    params->dsi.PLL_CK_CMD = 335;    
    params->dsi.PLL_CK_VDO = 335;     
    params->dsi.CLK_HS_POST = 36;
    params->dsi.CLK_HS_PRPR = 6;
    params->dsi.HS_TRAIL = 7;
    params->dsi.LPX = 5;

    params->dsi.clk_lp_per_line_enable = 0;
    params->dsi.esd_check_enable = 1;
    params->dsi.customization_esd_check_enable = 0;
    params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
    params->dsi.lcm_esd_check_table[0].count = 1;
    params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
  
}



static void lcm_reset(void)
{
    SET_RESET_PIN(1);
    MDELAY(5);
    SET_RESET_PIN(0);
    MDELAY(15);
    SET_RESET_PIN(1);
    MDELAY(120);

    LCM_LOGI("lcm reset end.\n");
}

static void lcm_init_power(void)
{
    #if 0
        LCM_LOGI("%s, begin\n", __func__);
        hwPowerOn(MT6325_POWER_LDO_VGP1, VOL_DEFAULT, "LCM_DRV");
        LCM_LOGI("%s, end\n", __func__);
    #endif
}

static void lcm_suspend_power(void)
{
    #if 0
        LCM_LOGI("%s, begin\n", __func__);
        hwPowerDown(MT6325_POWER_LDO_VGP1, "LCM_DRV");
        LCM_LOGI("%s, end\n", __func__);
    #endif
}

static void lcm_resume_power(void)
{
    #if 0
        LCM_LOGI("%s, begin\n", __func__);
        hwPowerOn(MT6325_POWER_LDO_VGP1, VOL_DEFAULT, "LCM_DRV");
        LCM_LOGI("%s, end\n", __func__);
    #endif
}

static void lcm_init(void)
{
    unsigned char cmd = 0x0;
    unsigned char data = 0x0F;  //up to +/-5.5V
    int ret = 0;
    LCM_LOGI("%s: truly_Nt35521Z\n",__func__);

    ret = NT50358A_write_byte(cmd, data);
    if (ret < 0)
        LCM_LOGI("nt35521z-truly----nt50358a----cmd=%0x--i2c write error----\n", cmd);
    else
        LCM_LOGI("nt35521z-truly----nt50358a----cmd=%0x--i2c write success----\n", cmd);
    cmd = 0x01;
    data = 0x0F;
   ret = NT50358A_write_byte(cmd, data);
    if (ret < 0)
        LCM_LOGI("nt35521z-truly----nt50358a----cmd=%0x--i2c write error----\n", cmd);
    else
        LCM_LOGI("nt35521z-truly----nt50358a----cmd=%0x--i2c write success----\n", cmd);
    set_gpio_lcd_enp(1);
    MDELAY(2);
    set_gpio_lcd_enn(1);
    MDELAY(2);

    lcm_reset();

		push_table(NULL, init_setting,
		sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
#ifndef MACH_FPGA

//    printk(KERN_ERR "yzwyzw %s %d\n",__func__,__LINE__);
    push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
    MDELAY(30);
//    SET_RESET_PIN(0);
    LCM_LOGI("%s,truly NT35521Z lcm_suspend power shut start\n", __func__);
    MDELAY(1);
    set_gpio_lcd_enn(0);
    MDELAY(1);
//    set_gpio_lcd_enn(0);
//    MDELAY(1);
//    set_gpio_lcd_enp(0);
//    MDELAY(1);
    set_gpio_lcd_enp(0);
#endif
    LCM_LOGI("%s,truly NT35521Z lcm_suspend power shutted over\n", __func__);

//    MDELAY(10); 
}

static void lcm_resume(void)
{
    LCM_LOGI("%s start: truly NT35521Z\n",__func__);
    lcm_init();
    LCM_LOGI("%s end: truly NT35521Z\n",__func__);
}


//static struct LCM_setting_table read_id[] = {
//    {0xFF, 3, {0x98,0x81,0x01} },
//    {REGFLAG_END_OF_TABLE, 0x00, {} }
//};



static unsigned int lcm_ata_check(unsigned char *buffer)
{
    return 0;
}

static void lcm_setbacklight(void *handle, unsigned int level)
{
    if (level > 255)
        level = 255;
    if (level < 2 && level !=0)
        level = 2;
    bl_level[0].para_list[0] = level;
    LCM_LOGI("wzx%s,backlight: level = %d \n", __func__, level);
    push_table(handle, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}



LCM_DRIVER nt35521z_hd_dsi_vdo_truly_lcm_drv = {
    .name = "nt35521z_hd_dsi_vdo_truly",
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
