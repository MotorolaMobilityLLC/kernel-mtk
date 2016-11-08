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

#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include "lcm_drv.h"
/* --------------------------------------------------------------------------- */
/* Local Constants */
/* --------------------------------------------------------------------------- */

#define FRAME_WIDTH									(720)
#define FRAME_HEIGHT								(1280)

#define REGFLAG_DELAY								 0xFE
#define REGFLAG_END_OF_TABLE						 0xFF	/* END OF REGISTERS MARKER */

#define LCM_DSI_CMD_MODE							  0
#define LCT_LCM_MAPP_BACKLIGHT						  1
/* --------------------------------------------------------------------------- */
/* Local Variables */
/* --------------------------------------------------------------------------- */

static LCM_UTIL_FUNCS lcm_util = { 0 };

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)	 lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)					 lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)		 lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg					 lcm_util.dsi_read_reg()

#ifdef CONFIG_LCT_LCM_GPIO_UTIL
	#define set_gpio_lcd_enp(cmd) lcm_util.set_gpio_lcd_enp_bias(cmd)
	#define set_gpio_lcd_enn(cmd) lcm_util.set_gpio_lcd_enn_bias(cmd)
	#define set_gpio_led_en(cmd) lcm_util.set_gpio_led_en_bias(cmd)
#else
	#define set_gpio_lcd_enp(cmd) 
	#define set_gpio_lcd_enn(cmd) 
#endif
#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define I2C_ID_NAME "I2C_LCD_BIAS"
#define TPS_ADDR 0x3E
#define TPS_I2C_BUSNUM 2
/***************************************************************************** 
* GLobal Variable
*****************************************************************************/
static struct i2c_board_info __initdata tps65132_board_info = {I2C_BOARD_INFO(I2C_ID_NAME, TPS_ADDR)};
static struct i2c_client *tps65132_i2c_client = NULL;


/***************************************************************************** 
* Function Prototype
*****************************************************************************/ 
static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tps65132_remove(struct i2c_client *client);
/***************************************************************************** 
* Data Structure
*****************************************************************************/

struct tps65132_dev	{	
struct i2c_client	*client;

};

static const struct i2c_device_id tps65132_id[] = {
{ I2C_ID_NAME, 0 },
{ }
};


static struct i2c_driver tps65132_iic_driver = {
    .id_table	= tps65132_id,
    .probe		= tps65132_probe,
    .remove		= tps65132_remove,
    .driver		= {
    .owner	= THIS_MODULE,
    .name	= "tps65132",
},
};
/***************************************************************************** 
* Extern Area
*****************************************************************************/ 

/***************************************************************************** 
* Function
*****************************************************************************/ 
static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id)
{  

    tps65132_i2c_client  = client;		
    return 0;      
}


static int tps65132_remove(struct i2c_client *client)
{  	
    tps65132_i2c_client = NULL;
    i2c_unregister_device(client);
    return 0;
}


int tps65132_write_bytes(unsigned char addr, unsigned char value)
{	
	int ret = 0;
    char write_data[2];	
	struct i2c_client *client = tps65132_i2c_client;
	if(client == NULL)
	{
		return 0;
	}
	
    
    write_data[0]= addr;
    write_data[1] = value;
    ret=i2c_master_send(client, write_data, 2);
    return ret ;
}
EXPORT_SYMBOL_GPL(tps65132_write_bytes);


static int __init tps65132_iic_init(void)
{
	int ret = 0;
	ret = i2c_register_board_info(TPS_I2C_BUSNUM, &tps65132_board_info, 1);
	i2c_add_driver(&tps65132_iic_driver);
	return 0;
}

static void __exit tps65132_iic_exit(void)
{
    i2c_del_driver(&tps65132_iic_driver);  
}

module_init(tps65132_iic_init);
module_exit(tps65132_iic_exit);

MODULE_AUTHOR("Xiaokuan Shi");
MODULE_DESCRIPTION("MTK TPS65132 I2C Driver");
MODULE_LICENSE("GPL"); 


struct LCM_setting_table {
	unsigned cmd;
	unsigned char count;
	unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {

	//set EXTC
	{0xB9, 3, {0xFF,0x83,0x94}},

	//Set MIPI
	{0xBA, 6, {0x62,0x03,0x68,0x6B,0xB2,0xC0}},

	//set power
	{0xB1, 10, {0x48,0x12,0x72,0x09,0x32,0x54,0x71,0x71,0x57,0x47}},

	//SET display
	{0xB2, 6, {0x00,0x80,0x64,0x0C,0x0D,0x2F}},

	//SET CYC
	{0xB4, 21, {0x73,0x74,0x73,0x74,0x73,0x74,0x01,0x0C,0x86,
			           0x75,0x00,0x3F,0x73,0x74,0x73,0x74,0x73,0x74,0x01,0x0C,0x86}},

	// set VCOM
	{0xB6, 2, {0x60,0x60}},

	//set D3
	{0xD3, 33, {0x00,0x00,0x07,0x07,0x40,0x07,0x0C,0x00,0x08,
			           0x10,0x08,0x00,0x08,0x54,0x15,0x0A,0x05,0x0A,0x02,
			           0x15,0x06,0x05,0x06,0x47,0x44,0x0A,0x0A,0x4B,0x10,
			           0x07,0x07,0x0C,0x40}},
	//set GIP
	{0xD5, 44, {0x1A,0x1A,0x1B,0x1B,0x00,0x01,0x02,0x03,0x04,
			           0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x24,0x25,0x18,
			           0x18,0x26,0x27,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
			           0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x20,
			           0x21,0x18,0x18,0x18,0x18}},

	// D6
	{0xD6, 44, {0x1A,0x1A,0x1B,0x1B,0x07,0x06,0x05,0x04,0x03,
			           0x02,0x01,0x00,0x0B,0x0A,0x09,0x08,0x21,0x20,0x18,
			           0x18,0x27,0x26,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
			           0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x25,
			           0x24,0x18,0x18,0x18,0x18}},

	//SET Damma
	{0xE0, 58, {0x00,0x12,0x1E,0x25,0x28,0x2C,0x2F,0x2D,0x5B,
			           0x6A,0x7a,0x76,0x7d,0x8C,0x8F,0x90,0xBA,0xAA,0x9B,
					   0xA4,0xB3,0x59,0x57,0x5B,0x5F,0x63,0x6B,0x79,0x7F,
					   0x00,0x12,0x1E,0x25,0x28,0x2C,0x2F,0x2D,0x5B,0x6A,
					   0x79,0x76,0x7D,0x8B,0x8E,0x8F,0x80,0x8F,0x94,						0xA4,0xB4,0x59,0x58,0x5D,0x60,0x64,0x6D,0x79,0x7F}},

	//SET C0
	{0xC0, 2, {0x1F,0x31}},

	//SET panel
	{0xCC, 1, {0x07}},

	//SET D4
	{0xD4, 4, {0x02}},

	//SET BD
	{0xBD, 1, {0x02}},

	//set D8
	{0xD8, 12, {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
			           0xFF,0xFF,0xFF}},
	//set BD
	{0xBD, 1, {0x00}},
	//set BD
	{0xBD, 1, {0x01}},
	//set GAS
	{0xB1, 1, {0x00}},
	//set BD
	{0xBD, 1, {0x00}},
	//set ECO
	{0xC6, 1, {0xef}},
	
	// Display OFF
	{0x11, 0, {}},
	{REGFLAG_DELAY, 120, {}},
	// Display ON
	{0x29, 0, {}},
	{REGFLAG_DELAY, 10, {}},
	{0x51,1,{0xff}},
	{0x53,1,{0x24}},
	{0x55,1,{0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	/* Sleep Mode On */
	{ 0x28, 0, {} },
	{ REGFLAG_DELAY, 10, {} },
	{ 0x10, 0, {} },
	{ REGFLAG_DELAY, 120, {} },
	{ REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table lcm_backlight_level_setting[] = {
	{ 0x51, 1, {0xFF} },
	{ REGFLAG_END_OF_TABLE, 0x00, {} }
};


static void push_table(struct LCM_setting_table *table, unsigned int count,
		       unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {

		unsigned cmd;

		cmd = table[i].cmd;

		switch (cmd) {

		case REGFLAG_DELAY:
			MDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}

}


/* --------------------------------------------------------------------------- */
/* LCM Driver Implementations */
/* --------------------------------------------------------------------------- */

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DSI;
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
#else
	params->dsi.mode = BURST_VDO_MODE;
#endif

	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_THREE_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	/* Not support in MT6573 */
	params->dsi.packet_size = 256;
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active				= 2;
	params->dsi.vertical_backporch					= 12;
	params->dsi.vertical_frontporch					= 15; //16
	params->dsi.vertical_active_line				= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active				= 40; //60
	params->dsi.horizontal_backporch				= 40;  //81
	params->dsi.horizontal_frontporch				= 40;  //81
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 150;	/* this value must be in MTK suggested table */
#else
	params->dsi.PLL_CLOCK = 260;	/* this value must be in MTK suggested table */
#endif


	params->dsi.cont_clock = 0;
	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd = 0x53;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x24;
}

void tps65132_enable(char en)
{
	if (en)
	{
		set_gpio_led_en(1);
		MDELAY(5);
		set_gpio_lcd_enp(1);
		MDELAY(12);
		set_gpio_lcd_enn(1);
		MDELAY(12);
		tps65132_write_bytes(0x00, 0x0f);
		tps65132_write_bytes(0x01, 0x0f);
	}
	else
	{
		tps65132_write_bytes(0x03, 0x40);
		set_gpio_led_en(0);
		MDELAY(5);
		set_gpio_lcd_enp(0);
		MDELAY(12);
		set_gpio_lcd_enn(0);
		MDELAY(12);

	}	
}

static void lcm_init(void)
{
	tps65132_enable(1);
	SET_RESET_PIN(1);
	MDELAY(20);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(120);
	push_table(lcm_initialization_setting,
		   sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
	push_table(lcm_deep_sleep_mode_in_setting,
		   sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
	tps65132_enable(0);
}


static void lcm_resume(void)
{
	MDELAY(10);
	lcm_init();
	MDELAY(10);
}


static void lcm_setbacklight(unsigned int level)
{
	
    #if(LCT_LCM_MAPP_BACKLIGHT)
	unsigned int mapped_level = 0;
	if (level > 255)
		level = 200;
	if (level > 102)
		mapped_level = (641*level + 36667)/(1000);
	else
		mapped_level=level;
	#endif
	/* Refresh value of backlight level. */
	lcm_backlight_level_setting[0].para_list[0] = mapped_level;
	push_table(lcm_backlight_level_setting,
		   sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}


LCM_DRIVER lct_hx8394f_booyitech_720p_vdo_lcm_drv = {

	.name = "lct_hx8394f_booyitech_720p_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.set_backlight = lcm_setbacklight,

};
