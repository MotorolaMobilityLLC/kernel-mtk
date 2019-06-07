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

#define LCM_PHYSICAL_WIDTH                  (68040)
#define LCM_PHYSICAL_HEIGHT                    (136080)
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

#ifndef HX83102_DELAY
#define HX83102_DELAY 4 
#endif

#ifndef HX83102_DELAY_BACK
#define HX83102_DELAY_BACK 6 
#endif

static unsigned int ontim_resume = 0;
struct LCM_setting_table {
    unsigned int cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
    {0x28,1, {0x00} },
    {REGFLAG_DELAY, 20, {} },
    {0x10, 1, {0x00} },
    {REGFLAG_DELAY, 120, {} },
//    {0x04, 1, {0x5a} },
//    {REGFLAG_DELAY, 5, {} },
//    {0x05, 1, {0x5a} },
//    {REGFLAG_DELAY, 20, {} },
};

static struct LCM_setting_table init_setting[] = {
    {0xb9,3,{0x83,0x10,0x2b}},
    {0x11,0,{}},
    {REGFLAG_DELAY, 120, {} },
    {0x29,0,{}},
    {REGFLAG_DELAY, 20, {} },
    {0x51,2,{0x00,0x00}},
    {0xc9,4,{0x08,0x0b,0xb8,0x01}},
    {0x55,1,{0x00}},
    {0x53,1,{0x2c}},
    {0x35,1,{0x00}},
    {0x36,1,{0x03}},
    {0xCE,1,{0x04}},
    {0xE6,60,{0x00,0x00,0x00,0x00,0xAA,0x00,0x00,0x00,0x00,0xAA,
		0x00,0x00,0x00,0x00,0xAA,0x00,0x00,0x00,0x00,0xAA,
		0x00,0x00,0x00,0x00,0xAA,0xCD,0x99,0xCD,0x00,0xBE,
		0xCD,0x99,0xCD,0x00,0xBE,0xCD,0x99,0xCD,0x00,0xBE,
		0x00,0x00,0x00,0x00,0xAA,0x00,0x00,0x00,0x00,0xAA,
		0x00,0x00,0x00,0x00,0xAA,0x00,0x00,0x00,0x00,0xAA}},
  {REGFLAG_END_OF_TABLE, 0x00, {} }

};
static struct LCM_setting_table bl_level[] = {
    {0x51, 2, {0x07,0xff} },
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
    LCM_LOGI("%s: ontim_HX83102_boyi\n",__func__);
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
    params->dsi.vertical_sync_active = 2; //old is 2,now is 4
    params->dsi.vertical_backporch = 15; //old is 8,now is 100
    params->dsi.vertical_frontporch = 255; //old is 15,now is 24
    params->dsi.vertical_active_line = FRAME_HEIGHT;
    params->dsi.horizontal_sync_active = 12;
    params->dsi.horizontal_backporch = 12;
    params->dsi.horizontal_frontporch = 12;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;
    params->dsi.ssc_disable = 0;
    params->dsi.ssc_range = 3;
    //params->dsi.HS_TRAIL = 15;
    params->dsi.PLL_CLOCK = 336;    /* this value must be in MTK suggested table */
    params->dsi.PLL_CK_CMD = 336;    
    params->dsi.PLL_CK_VDO = 336;     
    params->dsi.CLK_HS_POST = 36;

    params->dsi.clk_lp_per_line_enable = 0;
    params->dsi.esd_check_enable = 1;
    params->dsi.customization_esd_check_enable = 1;
    params->dsi.lcm_esd_check_table[0].cmd = 0xd6;
    params->dsi.lcm_esd_check_table[0].count = 5;
    params->dsi.lcm_esd_check_table[0].para_list[0] = 0x18;
    params->dsi.lcm_esd_check_table[0].para_list[1] = 0x18;
    params->dsi.lcm_esd_check_table[0].para_list[2] = 0x3a;
    params->dsi.lcm_esd_check_table[0].para_list[3] = 0x3a;
    params->dsi.lcm_esd_check_table[0].para_list[4] = 0x19;
    params->dsi.lcm_esd_check_table[0].type = 1;
    params->dsi.lcm_esd_check_table[1].cmd = 0x09;
    params->dsi.lcm_esd_check_table[1].count = 3;
    params->dsi.lcm_esd_check_table[1].para_list[0] = 0x81;
    params->dsi.lcm_esd_check_table[1].para_list[1] = 0xf3;
    params->dsi.lcm_esd_check_table[1].para_list[2] = 0x06;
    params->dsi.lcm_esd_check_table[1].type = 1;
    params->dsi.lcm_esd_check_table[2].cmd = 0xb1;
    params->dsi.lcm_esd_check_table[2].count = 4;
    params->dsi.lcm_esd_check_table[2].para_list[0] = 0x04;
    params->dsi.lcm_esd_check_table[2].para_list[1] = 0x2a;
    params->dsi.lcm_esd_check_table[2].para_list[2] = 0x27;
    params->dsi.lcm_esd_check_table[2].para_list[3] = 0x27;
    params->dsi.lcm_esd_check_table[2].type = 1;
  
}


#define CTP_RST_GPIO_PINCTRL_ONTIM 0
static void lcm_reset(void)
{
    //printf("[uboot]:lcm reset start.\n");
    
    if(ontim_resume){
        ontim_resume = 0;
//        if(tpd_gpio_output != NULL)
        MDELAY(1);
        tpd_gpio_output(CTP_RST_GPIO_PINCTRL_ONTIM,0);
        MDELAY(10);
        tpd_gpio_output(CTP_RST_GPIO_PINCTRL_ONTIM,1);
    }
    SET_RESET_PIN(1);
    MDELAY(1);
    SET_RESET_PIN(0);
    MDELAY(2);
    SET_RESET_PIN(1);
    MDELAY(50);

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
    LCM_LOGI("%s: ontim_HX83102_boyi\n",__func__);

    set_gpio_lcd_enp(1);
    MDELAY(2);
    set_gpio_lcd_enn(1);
    MDELAY(2);

    lcm_reset();

    push_table(NULL, init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
#ifndef MACH_FPGA

//    printk(KERN_ERR "yzwyzw %s %d\n",__func__,__LINE__);
    push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
    MDELAY(30);
//    SET_RESET_PIN(0);
    LCM_LOGI("%s,ontim_HX83102_boyilcm_suspend power shut start\n", __func__);
    MDELAY(1);
    set_gpio_lcd_enn(0);
    MDELAY(1);
//    set_gpio_lcd_enn(0);
//    MDELAY(1);
//    set_gpio_lcd_enp(0);
//    MDELAY(1);
    set_gpio_lcd_enp(0);
#endif
    LCM_LOGI("%s,ontim_HX83102_boyilcm_suspend power shutted over\n", __func__);

//    MDELAY(10); 
}

static void lcm_resume(void)
{
    LCM_LOGI("%s start: ontim_HX83102_boyi\n",__func__);
    ontim_resume = 1;
    lcm_init();
    LCM_LOGI("%s end: ontim_HX83102_boyi\n",__func__);
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
    bl_level[0].para_list[0] = (level>>4) & 0x0F;
    bl_level[0].para_list[1] = (level & 0x0F)<<4;
    LCM_LOGI("wzx%s,backlight: level = %d, set to 0x%x,0x%x \n", __func__, level,bl_level[0].para_list[0],bl_level[0].para_list[1]);
//    printk(KERN_ERR "yzw%s,backlight: level = %d, set to 0x%x,0x%x \n", __func__, level,bl_level[6].para_list[0],bl_level[6].para_list[1]);
    push_table(handle, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}



LCM_DRIVER HX83102_dsi_cmd_lcm_drv = {
    .name = "ontim_HX83102_boyi",
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
