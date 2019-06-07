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
#include "disp_dts_gpio.h"
#include "../../../pmic/include/mt6357/mtk_pmic_api.h"
#endif

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif



static LCM_UTIL_FUNCS lcm_util = { 0 };

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))
//#define SET_BLEN_PIN(v)     (lcm_util.set_blen_pin((v)))
//#define SET_RS_PIN(v)		(lcm_util.set_rs_pin((v)))
//#define SET_CS_PIN(v)		(lcm_util.set_cs_pin((v)))
#define SET_BL_KPAD_PIN(v)     (lcm_util.set_bl_kpad_pin((v)))
#define MDELAY(n)       (lcm_util.mdelay(n))
#define UDELAY(n)       (lcm_util.udelay(n))

static inline void send_ctrl_cmd(unsigned int cmd)
{
	lcm_util.send_cmd(cmd);
}


static inline void send_data_cmd(unsigned int data)
{
	lcm_util.send_data(data & 0xFF);
}




#ifndef ASSERT
#define ASSERT(expr)					\
	do {						\
		if (expr)				\
			break;				\
		pr_debug("DDP ASSERT FAILED %s, %d\n",	\
		       __FILE__, __LINE__);		\
	} while (0)
#endif



/* static unsigned char lcd_id_pins_value = 0xFF; */

#define FRAME_WIDTH                                     (240)
#define FRAME_HEIGHT                                    (320)

#define LCM_PHYSICAL_WIDTH                  (36720)
#define LCM_PHYSICAL_HEIGHT                    (48960)
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

/**
 * LCM Driver Implementations
 */
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	printk("lcm_set_util_funcs\n");
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
	printk("lcm_get_params\n");
	memset(params, 0, sizeof(LCM_PARAMS));
	params->type = LCM_TYPE_DBI;
	params->ctrl = LCM_CTRL_SERIAL_DBI;
	params->dbi.ctrl = LCM_CTRL_SERIAL_DBI;
	params->lcm_if = LCM_INTERFACE_DBI0;
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->physical_width = LCM_PHYSICAL_WIDTH/1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT/1000;


	params->dbi.port = 0;
	params->dbi.clock_freq = LCM_DBI_CLOCK_FREQ_125M;
	params->dbi.data_width = LCM_DBI_DATA_WIDTH_8BITS; 
	params->dbi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dbi.data_format.trans_seq = LCM_DBI_TRANS_SEQ_MSB_FIRST;
	params->dbi.data_format.padding = LCM_DBI_PADDING_ON_LSB;
	params->dbi.data_format.format = LCM_DBI_FORMAT_RGB666;
	params->dbi.data_format.width = LCM_DBI_DATA_WIDTH_8BITS;
	params->dbi.cpu_write_bits = LCM_DBI_CPU_WRITE_8_BITS;
	params->dbi.io_driving_current = LCM_DRIVING_CURRENT_16MA;

	//ToDo: need check whether need set this params
	//msb_io_driving_current
	//ctrl_io_driving_current

	params->dbi.serial.cs_polarity = LCM_POLARITY_RISING;
	params->dbi.serial.clk_polarity = LCM_POLARITY_RISING;
	params->dbi.serial.clk_phase = LCM_CLOCK_PHASE_0; 
	params->dbi.serial.is_non_dbi_mode = 0;
	params->dbi.serial.clock_base = LCM_SERIAL_CLOCK_FREQ_125M;
	params->dbi.serial.clock_div = LCM_SERIAL_CLOCK_DIV_2;
	params->dbi.serial.wire_num = LCM_DBI_C_4WIRE;
	params->dbi.serial.datapin_num = LCM_DBI_C_1DATA_PIN;
	
	params->dbi.te_mode = LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity = LCM_POLARITY_RISING;

}
static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	
	unsigned short x0, y0, x1, y1;
	unsigned short h_X_start, l_X_start, h_X_end, l_X_end, h_Y_start, l_Y_start, h_Y_end,
	    l_Y_end;
	printk("lcm_update,x=%d,y=%d,width=%d,height=%d\n",x,y,width,height);

	x0 = (unsigned short)x;
	y0 = (unsigned short)y;
	x1 = (unsigned short)x + width - 1;
	y1 = (unsigned short)y + height - 1;

	h_X_start = (x0 & 0xFF00) >> 8;
	l_X_start = x0 & 0x00FF;
	h_X_end = (x1 & 0xFF00) >> 8;
	l_X_end = x1 & 0x00FF;

	h_Y_start = (y0 & 0xFF00) >> 8;
	l_Y_start = y0 & 0x00FF;
	h_Y_end = (y1 & 0xFF00) >> 8;
	l_Y_end = y1 & 0x00FF;

	send_ctrl_cmd(0x2A);
	send_data_cmd(h_X_start);
	send_data_cmd(l_X_start);
	send_data_cmd(h_X_end);
	send_data_cmd(l_X_end);

	send_ctrl_cmd(0x2B);
	send_data_cmd(h_Y_start);
	send_data_cmd(l_Y_start);
	send_data_cmd(h_Y_end);
	send_data_cmd(l_Y_end);

	send_ctrl_cmd(0x2C);
}



static void init_lcm_registers(void)
{
	send_ctrl_cmd(0xFE);
	send_ctrl_cmd(0xEF);

	send_ctrl_cmd(0x3A);	/* Control interface color format */
	send_data_cmd(0x06);	/* RGB565 18bit/pixel */

	send_ctrl_cmd(0x36);
	send_data_cmd(0x40);

	send_ctrl_cmd(0xA4);
	send_data_cmd(0x44);
	send_data_cmd(0x44);

	send_ctrl_cmd(0xA5);
	send_data_cmd(0x42);
	send_data_cmd(0x42);

	send_ctrl_cmd(0xAA);
	send_data_cmd(0x88);
	send_data_cmd(0x88);

	send_ctrl_cmd(0xE8);/*TE*/
//	send_data_cmd(0x11);
//send_data_cmd(0x71);/*60HZ*/
//	send_data_cmd(0x00);/*70HZ*/
//end_data_cmd(0x12);
//end_data_cmd(0x8b);/*60HZ*/

	//send_data_cmd(0x17);
	//send_data_cmd(0x40);/*30HZ*/

   send_data_cmd(0x11);
   send_data_cmd(0x0B);/*60HZ*/


	send_ctrl_cmd(0xE3);/*ps=01*/
	send_data_cmd(0x01);
	send_data_cmd(0x10);

	send_ctrl_cmd(0xFF);
	send_data_cmd(0x61);

	send_ctrl_cmd(0xAC);
	send_data_cmd(0x00);
	send_ctrl_cmd(0xAE);
	send_data_cmd(0x2B);
	
	send_ctrl_cmd(0x35);
	send_data_cmd(0x00);	/* te on */

	send_ctrl_cmd(0xAF);
	send_data_cmd(0x55);
	
	send_ctrl_cmd(0xad);
	send_data_cmd(0x33);	/* te on */

	send_ctrl_cmd(0xA6);
	send_data_cmd(0x2A);
	send_data_cmd(0x2A);

	send_ctrl_cmd(0xA7);
	send_data_cmd(0x2B);
	send_data_cmd(0x2B);

	send_ctrl_cmd(0xA8);
	send_data_cmd(0x18);
	send_data_cmd(0x18);

	send_ctrl_cmd(0xA9);
	send_data_cmd(0x2A);
	send_data_cmd(0x2A);

	/*gama*/
	send_ctrl_cmd(0xF0);
	send_data_cmd(0x02);
	send_data_cmd(0x00);
	send_data_cmd(0x00);
	send_data_cmd(0xA);
	send_data_cmd(0xE);
	send_data_cmd(0x10);

	send_ctrl_cmd(0xF1);
	send_data_cmd(0x1);
	send_data_cmd(0x2);
	send_data_cmd(0x0);
	send_data_cmd(0x28);
	send_data_cmd(0x35);
	send_data_cmd(0xF);

	send_ctrl_cmd(0xF2);
	send_data_cmd(0x10);
	send_data_cmd(0x9);
	send_data_cmd(0x35);
	send_data_cmd(0x3);
	send_data_cmd(0x3);
	send_data_cmd(0x43);

	send_ctrl_cmd(0xF3);
	send_data_cmd(0x11);
	send_data_cmd(0xB);
	send_data_cmd(0x54);
	send_data_cmd(0x5);
	send_data_cmd(0x3);
	send_data_cmd(0x60);

	send_ctrl_cmd(0xF4);
	send_data_cmd(0xC);
	send_data_cmd(0x17);
	send_data_cmd(0x16);
	send_data_cmd(0x11);
	send_data_cmd(0x12);
	send_data_cmd(0xF);

	send_ctrl_cmd(0xF5);
	send_data_cmd(0x8);
	send_data_cmd(0x17);
	send_data_cmd(0x17);
	send_data_cmd(0x2A);
	send_data_cmd(0x2A);
	send_data_cmd(0xF);
	/*end of gama*/

	send_ctrl_cmd(0x11);
	MDELAY(120);



//	send_ctrl_cmd(0x2C);	/* clear the screen with black color */


	lcm_update(0, 0, FRAME_WIDTH, FRAME_HEIGHT);

	send_ctrl_cmd(0x29);/*display on*/
	MDELAY(100);
	send_ctrl_cmd(0x2C);

}





static void lcm_init(void)
{

	printk("lcm_init kernel\n");
	//mt6357_upmu_set_rg_ldo_vldo28_en(1);
	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(20);
	SET_RESET_PIN(1);
	MDELAY(120);

	init_lcm_registers();
	SET_BL_KPAD_PIN(1);
}

static void lcm_suspend(void)
{
	printk("lcm_suspend kernel\n");
	//mt6357_upmu_set_rg_ldo_vldo28_en(0);
   // SET_BLEN_PIN(0);
	SET_BL_KPAD_PIN(0);
   	SET_RESET_PIN(0);
	send_ctrl_cmd(0xFE);
	send_ctrl_cmd(0xEF);
	send_ctrl_cmd(0x28);
	MDELAY(120);
	send_ctrl_cmd(0x10);
	MDELAY(150);

}



static void lcm_resume(void)
{
	printk("lcm_resume\n");
	mt6357_upmu_set_rg_ldo_vldo28_sw_op_en(1);
	mt6357_upmu_set_rg_ldo_vldo28_en(1);
    lcm_init();
	//send_ctrl_cmd(0xFE);
	//send_ctrl_cmd(0xEF);
	//send_ctrl_cmd(0x11);
	//MDELAY(120);
	//send_ctrl_cmd(0x29);
	//MDELAY(50);
}






static void lcm_setbacklight(void *handle, unsigned int level)
{   
#if 0
	SET_BLEN_PIN(1);
	printk("lcm_setbacklight = %d\n", level);

	if (level > 255)
		level = 255;

	send_ctrl_cmd(0x51);
	send_data_cmd(level);
#endif
}



LCM_DRIVER gc9305_dbi_c_4wire_lcm_drv = {
	.name = "gc9305_dbi_c_4wire",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params = lcm_get_params,
	.init = lcm_init,
    .suspend = lcm_suspend,
    .resume = lcm_resume,
//    .compare_id = lcm_compare_id,
    .set_backlight_cmdq = lcm_setbacklight,

    .update         = lcm_update,

};
