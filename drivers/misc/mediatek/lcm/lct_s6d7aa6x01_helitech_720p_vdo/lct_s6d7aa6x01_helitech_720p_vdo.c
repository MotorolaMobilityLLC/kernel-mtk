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
#define read_reg_v2(cmd, buffer, buffer_size)	lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

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

#ifdef CONFIG_LCT_CABC_MODE_SUPPORT
extern int cabc_mode_mode;
#define CABC_MODE_SETTING_UI	1
#define CABC_MODE_SETTING_MV	2
#define CABC_MODE_SETTING_DIS	3
#define CABC_MODE_SETTING_NULL	0
//static int reg_mode = 0;
#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#endif
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


static int tps65132_write_bytes(unsigned char addr, unsigned char value)
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
//EXPORT_SYMBOL_GPL(tps65132_write_bytes);


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
//4.0V
static struct LCM_setting_table lcm_initialization_setting[] = {
        {0xF0,2,{0x5A,0x5A}},
		{0xF1,2,{0x5A,0x5A}},
		{0xFC,2,{0x5A,0x5A}},
		{0xB1,6,{0x25,0x40,0xE8,0x10,0x00,0x22}},
		{0xB2,1,{0x41}},
		{0xF2,4,{0x10,0x10,0x1a,0x0D}},
		//{0xF5,18,{0x7A,0x64,0x10,0x6A,0x27,0x1D,0x4C,0x4C,0x03,0x03,0x04,0x22,0x11,0x31,0x50,0x2A,0x16,0x75}},	//modified by zhudaolong at 20170314
		{0xF6,7,{0x04,0x8C,0x0F,0x80,0x46,0x00,0x00}},
		{0xF7,22,{0x0C,0x25,0x24,0x01,0x01,0x1A,0x18,0x16,0x14,0x01,0x01,0x01,0x01,0x01,0x01,0x04,0x01,0x01,0x01,0x01,0x01,0x01}},
		{0xF8,22,{0x0D,0x25,0x24,0x01,0x01,0x1B,0x19,0x17,0x15,0x01,0x01,0x01,0x01,0x01,0x01,0x05,0x01,0x01,0x01,0x01,0x01,0x01}},
		{0xED,11,{0xC0,0x58,0x38,0x58,0x38,0x58,0x38,0x04,0x04,0x11,0x22}},
		{0xEE,24,{0x67,0x89,0x45,0x23,0x55,0x55,0x55,0x55,0x43,0x32,0x87,0xA9,0x33,0x33,0x33,0x33,0x89,0x01,0x00,0x00,0x01,0x89,0x00,0x00}},
		{0xEF,32,{0x3D,0x09,0x00,0x40,0x06,0x67,0x45,0x23,0x01,0x00,0x00,0x00,0x00,0x04,0x80,0x80,0x84,0x07,0x09,0x89,0x08,0x12,0x21,0x21,0x03,0x03,0x44,0x33,0x00,0x00,0x00,0x00}},
		{0xFA,17,{0x10,0x30,0x18,0x1F,0x16,0x1B,0x20,0x1E,0x20,0x28,0x2B,0x2A,0x2A,0x28,0x27,0x26,0x2C}},
		{0xFB,17,{0x10,0x30,0x18,0x1F,0x16,0x1B,0x20,0x1E,0x20,0x28,0x2B,0x2A,0x2A,0x28,0x27,0x26,0x2C}},
		{0xC5,1,{0x21}},
		{0xFE,1,{0x48}},
		{0xC8,2,{0x24,0x53}},
		{0xC0,1,{0x03}},

	    {0x11, 0, {}},
        {REGFLAG_DELAY, 120, {}},

        //{0xB6, 2, {0x76,0x76}},

	    {0x29, 0, {}},
        {REGFLAG_DELAY, 20, {}},
		//{0xC9, 3, {0x13,0x00,0x14}},//pwm 20k
        {0x51, 2, {0x00,0x00}},	//modified by zhudaolong 20170421
        {0x53, 1, {0x24}},
        {0x55, 1, {0x01}},
        {REGFLAG_DELAY, 5, {}},
        
	      {REGFLAG_END_OF_TABLE, 0x00, {}}

	
};

#ifdef CONFIG_LCT_CABC_MODE_SUPPORT
static struct LCM_setting_table lcm_setting_ui[] = {
	//{0x51,1,{0xff}},
	{0x53,1,{0x24}},
	{0x55,1,{0x01}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}

};

static struct LCM_setting_table lcm_setting_mv[] = {
	//{0x51,1,{0xff}},
	{0x53,1,{0x2c}},
	{0x55,1,{0x03}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}

};

static struct LCM_setting_table lcm_setting_dis[] = {
	//{0x51,1,{0xff}},
	{0x53,1,{0x24}},
	{0x55,1,{0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}

};
#endif
static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	/* Sleep Mode On */
	{ 0x28, 0, {} },
	{ REGFLAG_DELAY, 50, {} },
	{ 0x10, 0, {} },
	{ REGFLAG_DELAY, 120, {} },
	{ REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table lcm_backlight_level_setting[] = {
	{ 0x51, 2, {0xff,0x0f} },
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

#ifdef CONFIG_LCT_CABC_MODE_SUPPORT
static void push_table_v22(void *handle, struct LCM_setting_table *table, unsigned int count,
		       unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {

		unsigned cmd;
		void *cmdq = handle;
		cmd = table[i].cmd;

		switch (cmd) {

		case REGFLAG_DELAY:
			MDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V22(cmdq, cmd, table[i].count, table[i].para_list, force_update);
		}
	}

}
#endif

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
	params->physical_width = 68;
	params->physical_height = 121;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
#else
	params->dsi.mode = BURST_VDO_MODE;
#endif

#ifdef CONFIG_LCT_DEVINFO_SUPPORT
	params->module="helitech";
	params->vendor="helitech";
	params->ic="s6d7aa6x01";
	params->info="720*1280";
#endif

	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	/* Not support in MT6573 */
	params->dsi.packet_size = 256;
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active				= 4;
	params->dsi.vertical_backporch					= 12;
	params->dsi.vertical_frontporch					= 16; 
	params->dsi.vertical_active_line				= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active				= 16; //20
	params->dsi.horizontal_backporch				= 48;  //52
	params->dsi.horizontal_frontporch				= 48;  //52
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 150;	/* this value must be in MTK suggested table */
#else
	params->dsi.PLL_CLOCK = 220; //this value must be in MTK suggested table
#endif

	params->dsi.cont_clock=0;
	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd          = 0xD9;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x80;
	params->dsi.lcm_esd_check_table[1].cmd          = 0x09;
	params->dsi.lcm_esd_check_table[1].count        = 3;
	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x80;
	params->dsi.lcm_esd_check_table[1].para_list[1] = 0x73;
	params->dsi.lcm_esd_check_table[1].para_list[2] = 0x06;
	params->dsi.lcm_esd_check_table[2].cmd          = 0xE0;
	params->dsi.lcm_esd_check_table[2].count        = 10;
	params->dsi.lcm_esd_check_table[2].para_list[0] = 0x00;
	params->dsi.lcm_esd_check_table[2].para_list[1] = 0x00;
	params->dsi.lcm_esd_check_table[2].para_list[2] = 0x02;
	params->dsi.lcm_esd_check_table[2].para_list[3] = 0x06;
	params->dsi.lcm_esd_check_table[2].para_list[4] = 0x06;
	params->dsi.lcm_esd_check_table[2].para_list[5] = 0x08;
	params->dsi.lcm_esd_check_table[2].para_list[6] = 0x0a;
	params->dsi.lcm_esd_check_table[2].para_list[7] = 0x08;
	params->dsi.lcm_esd_check_table[2].para_list[8] = 0x0e;
	params->dsi.lcm_esd_check_table[2].para_list[9] = 0x1C;
	
}

static void tps65132_enable(char en)
{
	if (en)
	{
		
		set_gpio_lcd_enp(1);
		MDELAY(5);
		set_gpio_lcd_enn(1);
		MDELAY(5);
		tps65132_write_bytes(0x00, 0x0f);//5.5->5.2
		tps65132_write_bytes(0x01, 0x0f);
		
	}
	else
	{
		tps65132_write_bytes(0x03, 0x40);
		//set_gpio_led_en(0);
		//MDELAY(5);		
		set_gpio_lcd_enn(0);
		MDELAY(5);
		set_gpio_lcd_enp(0);
		MDELAY(5);

	}	
}

static void lcm_init(void)
{
	tps65132_enable(1);
	SET_RESET_PIN(1);
	MDELAY(5);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(55);
	push_table(lcm_initialization_setting,
		   sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);	
}


static void lcm_suspend(void)
{
	set_gpio_led_en(0);
	MDELAY(5);
	push_table(lcm_deep_sleep_mode_in_setting,
		   sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
	tps65132_enable(0);
}


static void lcm_resume(void)
{
	//lcm_init();	//	modified by zhudaolong at 20170323
	tps65132_enable(1);
	SET_RESET_PIN(1);
	MDELAY(5);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(20);
	push_table(lcm_initialization_setting,
		   sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static unsigned int last_level=0;
static unsigned int hbm_enable=0;
static void lcm_setbacklight(unsigned int level)
{
	unsigned int level_hight,level_low;	//modified by zhudaolong at 20170309
	#if(LCT_LCM_MAPP_BACKLIGHT)
	static int mapped_level = 0;
	mapped_level = (8009*level + 20232)*16/(10000);	//modified by zhudaolong at 20170310
	if(mapped_level < 2)		//add by zhudaolong 20170502
		mapped_level = 2;	//add by zhudaolong 20170502
	#endif
	if(hbm_enable ==0)
	{				
		set_gpio_led_en(1);
		MDELAY(5);
		/* Refresh value of backlight level. */
		//modified begin by zhudaolong at 20170309
		level_hight=(mapped_level & 0x0f00)>>8;
		level_low=(mapped_level & 0x00ff);
		lcm_backlight_level_setting[0].para_list[0] = level_low;
		lcm_backlight_level_setting[0].para_list[1] = level_hight;
		//modified end by zhudaolong at 20170309
		//lcm_backlight_level_setting[0].para_list[0] = mapped_level;
		push_table(lcm_backlight_level_setting,
			   sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
		
		
	}
	else
	{    
		set_gpio_led_en(1);
		MDELAY(5);
		//lcm_backlight_level_setting[0].para_list[0] = 255;
		//modified begin by zhudaolong at 20170309
		level_hight=(4095 & 0x0f00)>>8;
		level_low=(4095 & 0x00ff);
		lcm_backlight_level_setting[0].para_list[0] = level_low;
		lcm_backlight_level_setting[0].para_list[1] = level_hight;
		//modified end by zhudaolong at 20170309
	push_table(lcm_backlight_level_setting,
		   sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
		printk("lcm_setbacklight hbm_enable1\n");
	}
	last_level = mapped_level;
}

//add for ATA test by liuzhen
#ifndef BUILD_LK 
extern atomic_t ESDCheck_byCPU;	
#endif

static unsigned int lcm_ata_check(unsigned char *buffer)
{
#ifndef BUILD_LK 
	unsigned int ret = 0 ,ret1=2; 
	unsigned char x0_MSB = 0x4;//((x0>>8)&0xFF); 
	unsigned char x0_LSB = 0x1;//(x0&0xFF); 
	unsigned char x1_MSB = 0x3;//((x1>>8)&0xFF); 
	unsigned char x1_LSB = 0x5;//(x1&0xFF); 	
		
	unsigned int data_array[3]; 
	unsigned char read_buf[4]; 

	#ifdef BUILD_LK 
	printf("ATA check size = 0x%x,0x%x,0x%x,0x%x in lk\n",x0_MSB,x0_LSB,x1_MSB,x1_LSB); 
	#else 
	printk("ATA check size = 0x%x,0x%x,0x%x,0x%x in ker\n",x0_MSB,x0_LSB,x1_MSB,x1_LSB); 
	#endif 

		
	data_array[0]= 0x0005390A;//HS packet 
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0xe0; 
	data_array[2]= (x1_LSB); 
	dsi_set_cmdq(data_array, 3, 1); 

	data_array[0] = 0x00043700; 
	dsi_set_cmdq(data_array, 1, 1); 

	atomic_set(&ESDCheck_byCPU, 1);
	read_reg_v2(0xe0, read_buf, 4); 
	atomic_set(&ESDCheck_byCPU, 0);

	//if((read_buf[0] == x0_MSB) && (read_buf[1] == x0_LSB)&& (read_buf[2] == x1_MSB) && (read_buf[3] == x1_LSB)) 
	if((read_buf[0] == x0_MSB) && (read_buf[1] == x0_LSB)) 
		ret = 1; 
	else 
		ret = 0; 
	#ifdef BUILD_LK 
	printf("ATA read buf size = 0x%x,0x%x,0x%x,0x%x,ret= %d in lk\n",read_buf[0],read_buf[1],read_buf[2],read_buf[3],ret); 
	#else 
	printk("ATA read buf size = 0x%x,0x%x,0x%x,0x%x,ret= %d ret1= %d in ker\n",read_buf[0],read_buf[1],read_buf[2],read_buf[3],ret,ret1); 
	#endif 

	return ret; 
#endif   
} 

//add by yufangfang for hbm
#ifdef CONFIG_LCT_HBM_SUPPORT

static void lcm_setbacklight_hbm(unsigned int level)
{
	unsigned int high_level,low_level;
	if(level==0)
	{
		level = last_level;
		hbm_enable = 0;
	}
	else
	{
		hbm_enable = 1;
		level = level*16;
	}
	high_level = (0xff | level);
	low_level = level >> 8;
	lcm_backlight_level_setting[0].para_list[0] = high_level;
	lcm_backlight_level_setting[0].para_list[1] = low_level;
	push_table(lcm_backlight_level_setting,
		   sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
	printk("yufangfang setbacklight lcm level hbm = %d\n",level);
}
/*
int recognition_hbm(void)
{
	if(hbm_enable==1)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL_GPL(recognition_hbm)
*/
#endif
#ifdef CONFIG_LCT_CABC_MODE_SUPPORT
/*
int recognition_cabc_mode(void)
{
	return reg_mode;
}

EXPORT_SYMBOL_GPL(recognition_cabc_mode);
*/
static void lcm_cabc_cmdq(void *handle, unsigned int mode)
{

	//reg_mode = mode;
	switch(mode)
	{
		case CABC_MODE_SETTING_UI:
			{
				push_table_v22(handle,lcm_setting_ui,
			   sizeof(lcm_setting_ui) / sizeof(struct LCM_setting_table), 1);
			}
		break;
		case CABC_MODE_SETTING_MV:
			{
				push_table_v22(handle,lcm_setting_mv,
			   sizeof(lcm_setting_mv) / sizeof(struct LCM_setting_table), 1);
			}
		break;
		case CABC_MODE_SETTING_DIS:
			{
				push_table_v22(handle,lcm_setting_dis,
			   sizeof(lcm_setting_dis) / sizeof(struct LCM_setting_table), 1);
			}
		break;
		default:
		{
			push_table_v22(handle,lcm_setting_ui,
			   sizeof(lcm_setting_ui) / sizeof(struct LCM_setting_table), 1);
		}

	}
}
#endif

LCM_DRIVER lct_s6d7aa6x01_helitech_720p_vdo_lcm_drv = {

	.name = "lct_s6d7aa6x01_helitech_720p_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.set_backlight = lcm_setbacklight,
	.ata_check    = lcm_ata_check,
#ifdef CONFIG_LCT_CABC_MODE_SUPPORT
	.set_cabc_cmdq = lcm_cabc_cmdq,
#endif
#ifdef CONFIG_LCT_HBM_SUPPORT
	.set_backlight_hbm = lcm_setbacklight_hbm,
#endif
};
