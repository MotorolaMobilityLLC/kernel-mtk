/*******add oufeiguang lcm for A158 ---qiumeng@wind-mobi.com begin at 20161109********/
#define LOG_TAG "LCM"

#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif

#include "lcm_drv.h"
#include <mt-plat/mt_gpio.h>
#include <mach/gpio_const.h>

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
/***********add for extern avdd ---shenyong@wind-mobi.com---start*********/
#include <linux/i2c.h>
/***********add for extern avdd ---shenyong@wind-mobi.com---end*********/

//liujinzhou@wind-mobi.com add at 20161231 begin
#ifdef CONFIG_WIND_DEVICE_INFO
		extern char *g_lcm_name;
#endif
//liujinzhou@wind-mobi.com add at 20161231 end


#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_notice("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif


static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)        (lcm_util.set_reset_pin((v)))
#define MDELAY(n)               (lcm_util.mdelay(n))
#define set_gpio_lcd_enp(cmd) \
		lcm_util.set_gpio_lcd_enp_bias(cmd)
#define GPIO_LCD_BIAS_ENP_PIN         (GPIO119 | 0x80000000)

/* Local Functions */
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                      lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                  lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)                                       lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)


#define FRAME_WIDTH                         (720)
#define FRAME_HEIGHT                        (1280)

#define REGFLAG_DELAY                       0xFC
#define REGFLAG_END_OF_TABLE                0xFD

//qiumeng@wind-mobi.com begin at 20161114
#define LCM_ID_NT35512  0x19  
//qiumeng@wind-mobi.com end at 20161114

//oufeigang esd for A158---qiumeng@wind-mobi.com begin at 20161121
bool oufeiguang_esd = false;
//oufeigang esd for A158---qiumeng@wind-mobi.com end at 20161121
/* Local Variables */

/***********add for extern avdd ---shenyong@wind-mobi.com---start*********/
#define TPS_I2C_BUSNUM  I2C_I2C_LCD_BIAS_CHANNEL	/* for I2C channel 0 */
#define I2C_ID_NAME "tps65132"
#define TPS_ADDR 0x3E
/*****************************************************************************
 * GLobal Variable
 *****************************************************************************/
//static struct i2c_board_info tps65132_board_info __initdata = { I2C_BOARD_INFO(I2C_ID_NAME, TPS_ADDR) };

static struct i2c_client *tps65132_i2c_client;

/*****************************************************************************
 * Function Prototype
 *****************************************************************************/
static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tps65132_remove(struct i2c_client *client);
static unsigned int lcm_compare_id(void);

/*****************************************************************************
 * Data Structure
 *****************************************************************************/

struct tps65132_dev {
	struct i2c_client *client;

};

static const struct i2c_device_id tps65132_id[] = {
	{I2C_ID_NAME, 0},
	{}
};

static const struct of_device_id  tps65132_of_match[] = {
	{.compatible = "mediatek,i2c_lcd_bias"},
	{},
};


/* #if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)) */
/* static struct i2c_client_address_data addr_data = { .forces = forces,}; */
/* #endif */
static struct i2c_driver tps65132_iic_driver = {
	.id_table = tps65132_id,
	.probe = tps65132_probe,
	.remove = tps65132_remove,
	/* .detect               = mt6605_detect, */
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "tps65132",
		   .of_match_table = tps65132_of_match,
		   },
};

static int tps65132_remove(struct i2c_client *client)
{
	printk("tps65132_remove\n");
	tps65132_i2c_client = NULL;
	i2c_unregister_device(client);
	return 0;
}

int tps65132_write_bytes(unsigned char addr, unsigned char value)
{
	int ret = 0;
	struct i2c_client *client = tps65132_i2c_client;
	char write_data[2] = { 0 };

	printk("TPS: info==>name=%s addr=0x%x\n", client->name, client->addr);
	write_data[0] = addr;
	write_data[1] = value;
	if(client)
	ret = i2c_master_send(client, write_data, 2);
	if (ret < 0)
		LCM_LOGI("tps65132 write data fail !!\n");
	return ret;
}

static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	printk("tps65132_iic_probe\n");
	printk("TPS: info==>name=%s addr=0x%x\n", client->name, client->addr);
	tps65132_i2c_client = client;
	return 0;

}


static int __init tps65132_iic_init(void)
{
	printk("tps65132_iic_init\n");
	i2c_add_driver(&tps65132_iic_driver);
	printk("tps65132_iic_init success\n");
	return 0;
}

static void __exit tps65132_iic_exit(void)
{
	LCM_LOGI("tps65132_iic_exit\n");
	i2c_del_driver(&tps65132_iic_driver);
}

module_init(tps65132_iic_init);
module_exit(tps65132_iic_exit);


/***********add for extern avdd ---shenyong@wind-mobi.com---end*********/

struct LCM_setting_table {
	unsigned char cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	/* Display off sequence */
	{0x28, 1, {0x00}},
	{REGFLAG_DELAY, 20, {}},
	/* Sleep Mode On */
	{0x10, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_initialization_setting[] = {
	{0xFF,4,{0xAA,0x55,0x25,0x01}},
		  
	{0xFC,1,{0x08}},
	{0xFC,1,{0x00}},
		  
	{0x6F,1,{0x21}},
	{0xF7,1,{0x01}},
	{0x6F,1,{0x21}},
	{0xF7,1,{0x00}},
    
	//qiumeng@wind-mobi.com 20170122 begin		  
	{0x6F,1,{0x1A}},
	{0xF7,1,{0x05}},
					  
	{0xFF,4,{0xAA,0x55,0x25,0x00}},
		  
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x00}},
	{0xC8,1,{0x80}},
		  
	{0xB1,2,{0x60,0x21}},
		  
	{0xB6,1,{0x10}},
	{0x6F,1,{0x02}},
	{0xB8,1,{0x08}},
	{0xBB,2,{0x74,0x44}},
	{0xBC,2,{0x00,0x00}},
	{0xEC,1,{0x05}},
	//qiumeng@wind-mobi.com 20170122 end
		  
	{0xBD,5,{0x02,0xB0,0x20,0x20,0x00}},
		  
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x01}},
	{0xB0,2,{0x05,0x05}},
	{0xB1,2,{0x05,0x05}},
		  
	{0xBC,2,{0xA9,0x00}},
	{0xBD,2,{0xA9,0x00}},
		  
	{0xBE,1,{0x73}},
	{0xBF,1,{0x73}},
		  
	{0xCA,1,{0x00}},
	{0xC0,1,{0x04}},	
		  
	{0xB9,2,{0x56,0x56}},
	{0xB3,2,{0x37,0x37}},
		  
	{0xBA,2,{0x26,0x26}},
	{0xB4,2,{0x10,0x10}},
		  
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x02}},		
	//modify lcm gammar for A158---qiumeng@wind-mobi.com 20170103 begin		
	{0xEE,1,{0x01}},										   
	{0xB0,16,{0x00,0x80,0x00,0x95,0x00,0xb5,0x00,0xc5,0x00,0xE3,0x01,0x03,0x01,0x1A,0x01,0x42}},																																									   
	{0xB1,16,{0x01,0x65,0x01,0x9A,0x01,0xC2,0x02,0x05,0x02,0x3C,0x02,0x3E,0x02,0x6A,0x02,0x9C}},																																																																				  
	{0xB2,16,{0x02,0xC0,0x02,0xE6,0x03,0x07,0x03,0x45,0x03,0x60,0x03,0x75,0x03,0x8E,0x03,0xAE}},  
																										
	{0xB3,4,{0x03,0xD8,0x03,0xDA}}, 										
																																																																										
	//{0xBC,16,{0x00,0x3C,0x00,0x7E,0x00,0x9D,0x00,0xB5,0x00,0xC9,0x00,0xEA,0x01,0x03,0x01,0x2D}},								
																										 
	//{0xBD,16,{0x01,0x47,0x01,0x7E,0x01,0xA9,0x01,0xED,0x02,0x24,0x02,0x25,0x02,0x59,0x02,0x92}},
																				   
	//{0xBE,16,{0x02,0xB5,0x02,0xE6,0x03,0x0D,0x03,0x3A,0x03,0x57,0x03,0x7C,0x03,0x94,0x03,0xB2}},
		  
								 
	//{0xBF,4,{0x03,0xDE,0x03,0xE7}},
	//modify lcm gammar for A158---qiumeng@wind-mobi.com 20170103 end		 
		  
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x06}},
		  
	{0xB0,2,{0x29,0x2A}},
	{0xB1,2,{0x10,0x12}},
	{0xB2,2,{0x14,0x16}},
	{0xB3,2,{0x18,0x1A}},
	{0xB4,2,{0x08,0x0A}},
	{0xB5,2,{0x2E,0x2E}},
	{0xB6,2,{0x2E,0x2E}},
	{0xB7,2,{0x2E,0x2E}},
	{0xB8,2,{0x2E,0x00}},
	{0xB9,2,{0x2E,0x2E}},
		  
	{0xBA,2,{0x2E,0x2E}},
	{0xBB,2,{0x01,0x2E}},
	{0xBC,2,{0x2E,0x2E}},
	{0xBD,2,{0x2E,0x2E}},
	{0xBE,2,{0x2E,0x2E}},
	{0xBF,2,{0x0B,0x09}},
	{0xC0,2,{0x1B,0x19}},
	{0xC1,2,{0x17,0x15}},
	{0xC2,2,{0x13,0x11}},
	{0xC3,2,{0x2A,0x29}},
	{0xE5,2,{0x2E,0x2E}},
		  
	{0xC4,2,{0x29,0x2A}},
	{0xC5,2,{0x1B,0x19}},
	{0xC6,2,{0x17,0x15}},
	{0xC7,2,{0x13,0x11}},
	{0xC8,2,{0x01,0x0B}},
	{0xC9,2,{0x2E,0x2E}},
	{0xCA,2,{0x2E,0x2E}},
	{0xCB,2,{0x2E,0x2E}},
	{0xCC,2,{0x2E,0x09}},
	{0xCD,2,{0x2E,0x2E}},
		  
	{0xCE,2,{0x2E,0x2E}},
	{0xCF,2,{0x08,0x2E}},
	{0xD0,2,{0x2E,0x2E}},
	{0xD1,2,{0x2E,0x2E}},
	{0xD2,2,{0x2E,0x2E}},
	{0xD3,2,{0x0A,0x00}},
	{0xD4,2,{0x10,0x12}},
	{0xD5,2,{0x14,0x16}},
	{0xD6,2,{0x18,0x1A}},
	{0xD7,2,{0x2A,0x29}},
		  
	{0xE6,2,{0x2E,0x2E}},
		  
		  
	{0xD8,5,{0x00,0x00,0x00,0x00,0x00}},
	{0xD9,5,{0x00,0x00,0x00,0x00,0x00}},
		  
	{0xE7,1,{0x00}},
		 
	//qiumeng@wind-mobi.com 20170122 begin 
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x04}},
	{0xC3,1,{0x83}},
	//qiumeng@wind-mobi.com 20170122 end
	
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x03}},
	{0xB0,2,{0x00,0x00}},
	{0xB1,2,{0x00,0x00}},
	{0xB2,5,{0x05,0x00,0x00,0x00,0x00}},
	{0xB6,5,{0x05,0x00,0x00,0x00,0x00}},
	{0xB7,5,{0x05,0x00,0x00,0x00,0x00}},
	{0xBA,5,{0x57,0x00,0x00,0x00,0x00}},
	{0xBB,5,{0x57,0x00,0x00,0x00,0x00}},
	{0xC0,4,{0x00,0x00,0x00,0x00}},
	{0xC1,4,{0x00,0x00,0x00,0x00}},
	{0xC4,1,{0x60}},
	{0xC5,1,{0x40}},
	 
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x05}},
	{0xBD,5,{0x03,0x01,0x03,0x03,0x03}},
	{0xB0,2,{0x17,0x06}},
	{0xB1,2,{0x17,0x06}},
	{0xB2,2,{0x17,0x06}},
	{0xB3,2,{0x17,0x06}},
	{0xB4,2,{0x17,0x06}},
	{0xB5,2,{0x17,0x06}},
	{0xB8,1,{0x00}},
	{0xB9,1,{0x00}},
	{0xBA,1,{0x00}},
	{0xBB,1,{0x02}},
	{0xBC,1,{0x00}},
		  
		  
	{0xC0,1,{0x07}},
	{0xC4,1,{0x80}},
	{0xC5,1,{0xA4}},
	{0xC8,2,{0x05,0x30}},
	{0xC9,2,{0x01,0x31}},
	{0xCC,3,{0x00,0x00,0x3C}},
	{0xCD,3,{0x00,0x00,0x3C}},
		  
	{0xD1,5,{0x00,0x05,0x09,0x07,0x10}},
	{0xD2,5,{0x00,0x05,0x0E,0x07,0x10}},
		  
	{0xE5,1,{0x06}},
	{0xE6,1,{0x06}},
	{0xE7,1,{0x06}},
	{0xE8,1,{0x06}},
	{0xE9,1,{0x06}},
	{0xEA,1,{0x06}},
			  
	{0xED,1,{0x30}},
//oufeigang esd for A158---qiumeng@wind-mobi.com begin at 20161121
	{0x62,1,{0x01}},
//oufeigang esd for A158---qiumeng@wind-mobi.com end at 20161121	  
	{0x11,  0,  {}},
	{REGFLAG_DELAY, 120, {}},
	{0x29,  0,  {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}},
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {

			case REGFLAG_DELAY :
				if (table[i].count <= 10)
					MDELAY(table[i].count);
				else
					MDELAY(table[i].count);
				break;

			case REGFLAG_END_OF_TABLE :
				break;

			default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}

/* LCM Driver Implementations */
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
		memset(params, 0, sizeof(LCM_PARAMS));
	
		params->type = LCM_TYPE_DSI;
	    //modify panel physical size to 5 inches---qiumeng@wind-mobi.com 20170324 begin
		params->physical_width  = 60;
		params->physical_height = 112;
		//modify panel physical size to 5 inches---qiumeng@wind-mobi.com 20170324 end
		params->width = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;
	

		params->dsi.mode = SYNC_PULSE_VDO_MODE;
	//	params->dsi.switch_mode = CMD_MODE;

	//	params->dsi.switch_mode_enable = 0;
	
		/* DSI */
		/* Command mode setting */
		//qiumeng@wind-mobi.com 20170122 begin
		params->dsi.LANE_NUM = LCM_FOUR_LANE;
		//qiumeng@wind-mobi.com 20170122 end
		/* The following defined the fomat for data coming from LCD engine. */
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	
		/* Highly depends on LCD driver capability. */
		params->dsi.packet_size = 256;
		/* video mode timing */
	
		params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	    //improve lcm FPS for A158---qiumeng@wind-mobi.com 20170412 begin
		params->dsi.vertical_sync_active				= 4;
		params->dsi.vertical_backporch				= 18;//10
		params->dsi.vertical_frontporch				= 18;//10
		params->dsi.vertical_active_line				= FRAME_HEIGHT;
		params->dsi.ssc_disable                     =1;
		params->dsi.horizontal_sync_active			= 4;
		params->dsi.horizontal_backporch				= 70;//88
		params->dsi.horizontal_frontporch				= 70;//88
		params->dsi.horizontal_active_pixel			= FRAME_WIDTH;

		params->dsi.PLL_CLOCK = 216;//211
		//improve lcm FPS for A158---qiumeng@wind-mobi.com 20170412 end
//		params->dsi.CLK_HS_POST = 36;
//		params->dsi.clk_lp_per_line_enable = 0;
//qiumeng@wind-mobi.com begin at 20161114
		params->dsi.esd_check_enable = 1;
		params->dsi.customization_esd_check_enable = 1;
		params->dsi.lcm_esd_check_table[0].cmd = 0x0a;
		params->dsi.lcm_esd_check_table[0].count = 1;
		params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
//qiumeng@wind-mobi.com end at 20161114
//oufeigang esd for A158---qiumeng@wind-mobi.com begin at 20161121
        params->dsi.noncont_clock = 1;
        params->dsi.clk_lp_per_line_enable = 1;
//oufeigang esd for A158---qiumeng@wind-mobi.com end at 20161121

}


static unsigned int lcm_compare_id(void)
{

	unsigned int id = 0;
	unsigned char buffer[2];
	unsigned int array[16];

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);

	SET_RESET_PIN(1);
	MDELAY(20);

	array[0] = 0x00023700;  /* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);
//qiumeng@wind-mobi.com begin at 20161114
    read_reg_v2(0xda, buffer, 2);
//qiumeng@wind-mobi.com end at 20161114
	id = buffer[0];     /* we only need ID */

	LCM_LOGI("%s,nt35512 debug: nt35512 id = 0x%08x\n", __func__, id);
	
    //oufeigang esd for A158---qiumeng@wind-mobi.com begin at 20161121
	if (id == LCM_ID_NT35512)
	{
	    oufeiguang_esd = true;
		return 1;
	}
	else
		return 0;
	//oufeigang esd for A158---qiumeng@wind-mobi.com end at 20161121


}


static void lcm_init(void)
{
/***********add for extern avdd ---shenyong@wind-mobi.com---start*********/
	unsigned char cmd = 0x0;
	unsigned char data = 0xFF;
	int ret = 0;
	cmd = 0x00;
	data = 0x10;
/***********add for extern avdd ---shenyong@wind-mobi.com---end*********/
	/*-----------------DSV start---------------------*/
	//set_gpio_lcd_enp(1);
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);

/***********add for extern avdd ---shenyong@wind-mobi.com---start*********/
	MDELAY(5);
	ret = tps65132_write_bytes(cmd, data);
		if (ret < 0)
		printk("nt35521----tps6132----cmd=%0x--i2c write error----\n", cmd);
	else
		printk("nt35521----tps6132----cmd=%0x--i2c write success----\n", cmd);
	cmd = 0x01;
	data = 0x10;
	ret = tps65132_write_bytes(cmd, data);
		if (ret < 0)
		printk("nt35521----tps6132----cmd=%0x--i2c write error----\n", cmd);
	else
		printk("nt35521----tps6132----cmd=%0x--i2c write success----\n", cmd);
	MDELAY(5);
/***********add for extern avdd ---shenyong@wind-mobi.com---end*********/	
	/*-----------------DSV end---------------------*/

	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(10);

	SET_RESET_PIN(1);
	MDELAY(10);


	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);

	SET_RESET_PIN(0);

//	set_gpio_lcd_enp(0);
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);


	MDELAY(10);
}

static void lcm_resume(void)
{
	lcm_init();
}

//liujinzhou@wind-mobi.com modify at 20161230 begin
static unsigned int lcm_ata_check(unsigned char *buf)
{
	#ifdef CONFIG_WIND_DEVICE_INFO
	if(!strcmp(g_lcm_name,"nt35512_cmi_720p_oufeiguang_drv")){
		return 1;
	}else{
		return -1;
	}
	#endif
}
//liujinzhou@wind-mobi.com modify at 20161230 end

LCM_DRIVER nt35512_cmi_720p_oufeiguang_drv= {
	.name               = "nt35512_cmi_720p_oufeiguang_drv",
	.set_util_funcs     = lcm_set_util_funcs,
	.get_params         = lcm_get_params,
	.init               = lcm_init,
	.compare_id  = lcm_compare_id,
	.suspend            = lcm_suspend,
	.resume             = lcm_resume,
	.ata_check          = lcm_ata_check,    //sunsiyuan@wind-mobi.com add ata_check at 20161130
};
/*******add oufeiguang lcm for A158 ---qiumeng@wind-mobi.com end at 20161109********/
