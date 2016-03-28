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
#ifdef CONFIG_MTK_LEGACY
	#include <mach/mt_pm_ldo.h>
	#include <mach/mt_gpio.h>
#endif
#endif
#ifdef BUILD_LK
#define LCD_DEBUG(fmt)  dprintf(CRITICAL, fmt)
#else
#define LCD_DEBUG(fmt)  pr_debug(fmt)
#endif
#ifdef CONFIG_MTK_LEGACY
#include <cust_i2c.h>
#include <mach/gpio_const.h>
#include <cust_gpio_usage.h>
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (720)
#define FRAME_HEIGHT (1280)

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))
#define REGFLAG_DELAY             							0XFD
#define REGFLAG_END_OF_TABLE      							0xFE   // END OF REGISTERS MARKER


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)									lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)				lcm_util.dsi_write_regs(addr, pdata, byte_nums)
//#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#ifdef CONFIG_LCM_GPIO_UTIL
	#define set_gpio_lcd_enp(cmd) lcm_util.set_gpio_lcd_enp_bias(cmd)
	#define set_gpio_lcd_enn(cmd) lcm_util.set_gpio_lcd_enn_bias(cmd)
#else
	#define set_gpio_lcd_enp(cmd) 
	#define set_gpio_lcd_enn(cmd) 
#endif
//#define set_gpio_lcd_enn(cmd) lcm_util.set_gpio_lcd_enn_bias(cmd)


/*----------------------------------------add by caozhg----start--------------------------------------------------------*/
#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>  
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
//#include <linux/jiffies.h>
#include <linux/uaccess.h>
//#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
/***************************************************************************** 
 * Define
 *****************************************************************************/
#define TPS_I2C_BUSNUM  2//for I2C channel 0
#define I2C_ID_NAME "tps65132"
#define TPS_ADDR 0x3E
/***************************************************************************** 
 * GLobal Variable
 *****************************************************************************/
#ifndef CONFIG_OF
static struct i2c_board_info __initdata tps65132_board_info = {I2C_BOARD_INFO(I2C_ID_NAME, TPS_ADDR)};
#endif
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

#ifdef CONFIG_OF
static const struct of_device_id tps65132_of_match[] = {
	{ .compatible = "mediatek,I2C_LCD_BIAS", },
	{},
};
MODULE_DEVICE_TABLE(of, tps65132_of_match);
#endif


static const struct i2c_device_id tps65132_id[] = {{I2C_ID_NAME,0},{}};

//#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
//static struct i2c_client_address_data addr_data = { .forces = forces,};
//#endif
static struct i2c_driver tps65132_iic_driver = {
	.id_table	= tps65132_id,
	.probe		= tps65132_probe,
	.remove		= tps65132_remove,
	//.detect		= mt6605_detect,
	.driver		= {
		.name	= "tps65132",
		#ifdef CONFIG_OF 
       			.of_match_table = tps65132_of_match,
		#endif
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
	printk( "tps65132_iic_probe\n");
	printk("TPS: info==>name=%s addr=0x%x\n",client->name,client->addr);
	tps65132_i2c_client  = client;		
	return 0;      
}


static int tps65132_remove(struct i2c_client *client)
{  	
  printk( "tps65132_remove\n");
  tps65132_i2c_client = NULL;
   i2c_unregister_device(client);
  return 0;
}


int tps65132_write_bytes(unsigned char addr, unsigned char value)
{	
	int ret = 0;
	unsigned char xfers = 1;
	int retries = 3;//try 3 timers

	struct i2c_client *client = tps65132_i2c_client;
	unsigned char write_data[8];	
	write_data[0]= addr;
	memcpy(&write_data[1], &value, 1);
	do {
		struct i2c_msg msgs[1] = {
			{
				.addr = client->addr,
				.flags = 0,
				.len = 1 + 1,
				.buf = write_data,
			},
		};

		/*
		 * Avoid sending the segment addr to not upset non-compliant
		 * DDC monitors.
		 */
		ret = i2c_transfer(client->adapter, msgs, xfers);

		if (ret == -ENXIO) {
			printk("tps65132 write data fail !!\n");
			break;
		}
	} while (ret != xfers && --retries);

	return ret == xfers ? 1 : -1;
}
EXPORT_SYMBOL_GPL(tps65132_write_bytes);


/*
 * module load/unload record keeping
 */

static int __init tps65132_iic_init(void)
{

   printk( "tps65132_iic_init\n");
#ifdef CONFIG_OF
	//battery_log(BAT_LOG_CRTI, "[sm5414_init] init start with i2c DTS");
	printk ("[tps65132_iic_init] init start with i2c DTS");
#else
   i2c_register_board_info(TPS_I2C_BUSNUM, &tps65132_board_info, 1);
   printk( "tps65132_iic_init2\n");
#endif
   if (i2c_add_driver(&tps65132_iic_driver) != 0)
   {
	printk ("[tps65132_iic_init] failed to register i2c driver.\n");
	return -1;
   }
   printk( "tps65132_iic_init success\n");	
   return 0;
}

static void __exit tps65132_iic_exit(void)
{
  printk( "tps65132_iic_exit\n");
  i2c_del_driver(&tps65132_iic_driver);  
}


module_init(tps65132_iic_init);
module_exit(tps65132_iic_exit);

MODULE_AUTHOR("Xiaokuan Shi");
MODULE_DESCRIPTION("MTK TPS65132 I2C Driver");
MODULE_LICENSE("GPL"); 
#endif

/*----------------------------------------add by caozhg----end--------------------------------------------------------*/

//#define LCM_DSI_CMD_MODE

struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {
	
	/*
	Note :

	Data ID will depends on the following rule.
	
		count of parameters > 1	=> Data ID = 0x39
		count of parameters = 1	=> Data ID = 0x15
		count of parameters = 0	=> Data ID = 0x05

	Structure Format :

	{DCS command, count of parameters, {parameter list}}
	{REGFLAG_DELAY, milliseconds of time, {}},

	...

	Setting ending by predefined flag
	
	{REGFLAG_END_OF_TABLE, 0x00, {}}
	*/



//{0x10,0,{}},
{0xB9,3,{0xFF,0x83,0x94}},
{0xBA,6,{0x63,0x03,0x68,0x6B,0xB2,0xC0 }},
{0x11,0,{}},
{REGFLAG_DELAY, 120, {}},  
{0xB1,10,{0x48, 0x0a, 0x6a, 0x09, 0x33, 0x54, 0x71, 0x71, 0x2e, 0x45}}, 
//{0xB1,1,{0x60}},    
{0xCC,1,{0x0B}},          
{0xE0,58,{0x00,0x03,0x0A,0x10,0x11,0x15,0x19,0x15,0x2F,0x3F,0x4F,0x4F,0x5D,0x70,0x78,0x7E,0x8F,0x97,0x95,0xA2,0xB2,0x58,0x57,0x5D,0x63,0x67,0x6A,0x79,0x7F,0x00,0x03,0x0A,0x10,0x11,0x15,0x19,0x15,0x2F,0x3F,0x4F,0x4F,0x5D,0x70,0x78,0x7E,0x8F,0x97,0x95,0xA2,0xB2,0x58,0x57,0x5D,0x63,0x67,0x6A,0x79,0x7F}},   
{0xD2,1,{0x66}},   
//{0xB2,6,{0x00,0x80,0x64,0x0C,0x06,0x2F}},
{0xB2,6,{0x00,0x80,0x64,0x0e,0x0a,0x2F,0x00,0x00,0x00,0x00,0xc0,0x18}},//modify by caozhg 20160223
{0xB4,22,{0x20,0x7C,0x20,0x7C,0x20,0x7C,0x01,0x05,0xFF,0x35,0x00,0x3F,0x20,0x7C,0x20,0x7C,0x20,0x7C,0x01,0x05,0xFF,0x3F}},
{0xD3,33,{0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x10,0x32,0x10,0x01,0x00,0x01,0x32,0x13,0xC1,0x00,0x01,0x32,0x10,0x08,0x00,0x00,0x37,0x03,0x03,0x03,0x37,0x05,0x05,0x37,0x0C,0x40}},  
{0xD5,44,{0x18,0x18,0x18,0x18,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x19,0x19,0x19,0x19,0x20,0x21,0x22,0x23}}, 
{0xD6,44,{0x18,0x18,0x19,0x19,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x19,0x19,0x18,0x18,0x23,0x22,0x21,0x20}},   
{0xB6,2,{0x66,0x66}},  
{0xC0,2,{0x1F,0x73}},  
{0xD4,1,{0x02}},   
{0xBD,1,{0x01}},
//{0xB1,1,{0x60}},   
{0xB1,1,{0x00}},//modify by caozhg 20160223   
{0xBD,1,{0x00}},   
{0xBD,1,{0x00}},
{0xC1,43,{0x01,0x00,0x08,0x10,0x18,0x21,0x2A,0x31,0x39,0x40,0x46,0x4D,0x55,0x5C,0x64,0x6B,0x73,0x7A,0x82,0x8A,0x92,0x9B,0xA3,0xAC,0xB5,0xBC,0xC5,0xCD,0xD5,0xDE,0xE7,0xEF,0xF8,0xFF,0x1B,0x09,0x64,0x40,0x5B,0xB8,0x8B,0xA8,0xC0}},  
{0xBD,1,{0x01}},  
{0xC1,42,{0x00,0x08,0x10,0x19,0x21,0x2A,0x31,0x38,0x3F,0x45,0x4B,0x52,0x59,0x61,0x68,0x6F,0x76,0x7D,0x85,0x8D,0x95,0x9D,0xA4,0xAD,0xB5,0xBC,0xC4,0xCC,0xD4,0xDC,0xE4,0xED,0xF5,0x2C,0x87,0xAB,0xC1,0x34,0x1D,0x11,0x1C,0x40}},  
{0xBD,1,{0x02}},  
{0xC1,42,{0x00,0x07,0x0F,0x17,0x1E,0x27,0x2F,0x36,0x3D,0x43,0x49,0x50,0x56,0x5E,0x64,0x6C,0x72,0x7A,0x81,0x88,0x90,0x98,0xA0,0xA7,0xAF,0xB7,0xBE,0xC5,0xCD,0xD5,0xDC,0xE4,0xF4,0x34,0xA5,0x54,0xCC,0xC7,0x73,0xD7,0x8A,0x80}},  
{0xBD,1,{0x00}},
    
    {REGFLAG_DELAY, 100, {}},
    {0x29,0,{}},
    {REGFLAG_DELAY, 20, {}},


/*
    {REGFLAG_DELAY, 100, {}},
    {0xB9,3,{0xff,0x83,0x94}},
    {0x11,0,{}}, 
    {REGFLAG_DELAY, 120, {}},
    {0x29,0,{}},
    {REGFLAG_DELAY, 20, {}},
*/
	// Note
	// Strongly recommend not to set Sleep out / Display On here. That will cause messed frame to be shown as later the backlight is on.


	// Setting ending by predefined flag
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++) {
		
        unsigned cmd;
        cmd = table[i].cmd;
		
        switch (cmd) {
			
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE :
                break;
				
            default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
       	}
    }
	
}

#ifdef BUILD_LK
static struct mt_i2c_t hx8394f_i2c;
#define HX8394F_SLAVE_ADDR 0x3e
/**********************************************************
  *
  *   [I2C Function For Read/Write hx8394f] 
  *
  *********************************************************/
static unsigned int hx8394f_write_byte(unsigned char addr, unsigned char value)
{
    unsigned int ret_code = I2C_OK;
    unsigned char write_data[2];
    kal_uint16 len;

    write_data[0]= addr;
    write_data[1] = value;

    hx8394f_i2c.id = 2;
    /* Since i2c will left shift 1 bit, we need to set hx8394f I2C address to >>1 */
    hx8394f_i2c.addr = (HX8394F_SLAVE_ADDR);
    hx8394f_i2c.mode = ST_MODE;
    hx8394f_i2c.speed = 100;
    len = 2;

    ret_code = i2c_write(&hx8394f_i2c, write_data, len);
    printf("%s: i2c_write: ret_code: %d\n", __func__, ret_code);

    return ret_code;
}

#endif

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->physical_width = 62;
	params->physical_height = 111;
	// enable tearing-free
	//params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
	//params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
#ifdef CONFIG_SLT_DRV_DEVINFO_SUPPORT
	params->module="BV050HDM-A00-380W";
	params->vendor="BOE";
	params->ic="hx8394f";
	params->info="720*1280";
#endif

#if defined(LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
#else
	params->dsi.mode   = BURST_VDO_MODE;//SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;
#endif
	
	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	// Not support in MT6573

	//params->dsi.DSI_WMEM_CONTI=0x3C; 
	//params->dsi.DSI_RMEM_CONTI=0x3E; 

		
	params->dsi.packet_size=256;

	// Video mode setting		
	//params->dsi.intermediat_buffer_num = 2;

	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active				= 4;
	params->dsi.vertical_backporch					= 12;
	params->dsi.vertical_frontporch					= 16;
	params->dsi.vertical_active_line				= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active				= 60;
	params->dsi.horizontal_backporch				= 150;
	params->dsi.horizontal_frontporch				= 150;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	// Bit rate calculation
	params->dsi.PLL_CLOCK				= 255;
	//esd check 0x09->0x80 0x73 0x04     0xd9->0x80
	params->dsi.clk_lp_per_line_enable = 1;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd          = 0xd9;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x80;

	params->dsi.lcm_esd_check_table[1].cmd          = 0x09;
	params->dsi.lcm_esd_check_table[1].count        = 3;
	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x80;
	params->dsi.lcm_esd_check_table[1].para_list[1] = 0x73;
	params->dsi.lcm_esd_check_table[1].para_list[2] = 0x04;
	//params->dsi.lcm_esd_check_table[1].para_list[3] = 0x00;

	params->dsi.lcm_esd_check_table[2].cmd          = 0x45;
	params->dsi.lcm_esd_check_table[2].count        = 2;
	params->dsi.lcm_esd_check_table[2].para_list[0] = 0x05;
	params->dsi.lcm_esd_check_table[2].para_list[1] = 0x1e;

}
static void KTD2125_enable(char en)
{
	#ifdef BUILD_LK
	lcm_util.set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_LCD_BIAS_ENP_PIN_M_GPIO);
	lcm_util.set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_LCD_BIAS_ENN_PIN_M_GPIO);
	lcm_util.set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
	lcm_util.set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
	#endif
	if (en)
	{

		#ifdef BUILD_LK
		   lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
		   MDELAY(12);
		   lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);
		   MDELAY(12);
		   //printf("[LK]---cmd---hx8394f----%s------\n",__func__);
		   hx8394f_write_byte(0x00, 0x0f);
		   hx8394f_write_byte(0x01, 0x0f);
		#else
		   set_gpio_lcd_enp(1);
		   MDELAY(12);
		   set_gpio_lcd_enn(1);
		   MDELAY(12);
		   tps65132_write_bytes(0x00, 0x0f);
		   tps65132_write_bytes(0x01, 0x0f);
		#endif
	}
	else
	{
		#ifdef BUILD_LK
			lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);
			MDELAY(12);
			lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);
			MDELAY(12);
		#else
			set_gpio_lcd_enn(0);
			MDELAY(12);
			set_gpio_lcd_enp(0);
			MDELAY(12);
			tps65132_write_bytes(0x03, 0x33);

		#endif


	}
	
}

static unsigned int lcm_compare_id(void)
{

	int   array[4];
	char  buffer[3];
	char  id0=0;
	char  id1=0;
	char  id2=0;
	int   id=0;

        //KTD2125_enable(1);
        MDELAY(10);
        SET_RESET_PIN(1);
        MDELAY(20);
        SET_RESET_PIN(0);
        MDELAY(10);
        SET_RESET_PIN(1);
	MDELAY(120);

	//------------------B9h----------------//
		array[0] = 0x00043902; 						 
		array[1] = 0x9483FFB9; 				
	        dsi_set_cmdq(&array[0], 2, 1); 

	array[0] = 0x00013700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xDA,buffer, 1); 
	
	array[0] = 0x00013700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0xDB,buffer+1, 1);

	
	array[0] = 0x00013700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0xDC,buffer+2, 1);
	
	id0 = buffer[0]; //should be 0x00
	id1 = buffer[1];//should be 0xaa
	id2 = buffer[2];//should be 0x55
        KTD2125_enable(0);
#ifdef BUILD_LK
	printf("%s, id0 = 0x%08x\n", __func__, id0);//should be 0x00
	printf("%s, id1 = 0x%08x\n", __func__, id1);//should be 0xaa
	printf("%s, id2 = 0x%08x\n", __func__, id2);//should be 0x55
#endif
	id = ((id0<<16)|(id1 << 8)) | id2; //we only need ID
	if (id == 0x018101 || id == 0x1a5041 || id == 0x1a5042 || id == 0x1a5043)//otp  //if no vsp/vsn enable the id should be the default value 0x83940f
	//if (id == 0x83940f) //default
		return 1;
	else
		return 0;
}

static void lcm_init(void)
{
    //lcm_compare_id ();
    #ifdef BUILD_LK
		  printf("[LK]---cmd---hx8394f----%s------\n",__func__);
    #else
		  printk("[KERNEL]---cmd---hx8394f-----%s------\n",__func__);
    #endif
    KTD2125_enable(1);
    SET_RESET_PIN(1);
    MDELAY(20);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);

    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}



static void lcm_suspend(void)
{
	unsigned int data_array[2];

	data_array[0] = 0x00280500; // Display Off
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10); 
	data_array[0] = 0x00100500; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(100);
        KTD2125_enable(0);
#ifdef BUILD_LK
	printf("uboot %s\n", __func__);
#else
	printk("kernel %s\n", __func__);
#endif
}


static void lcm_resume(void)
{



	unsigned int data_array[2];
        KTD2125_enable(1);
	data_array[0] = 0x00110500; // Display Off
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120); 
	data_array[0] = 0x00290500; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);

#ifdef BUILD_LK
	printf("uboot %s\n", __func__);
#else
	printk("kernel %s\n", __func__);
#endif
//	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
	//lcm_init();
}

#if defined(LCM_DSI_CMD_MODE)
static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];
	
#ifdef BUILD_LK
	printf("uboot %s\n", __func__);
#else
	printk("kernel %s\n", __func__);	
#endif

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	data_array[3]= 0x00053902;
	data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[5]= (y1_LSB);
	data_array[6]= 0x002c3909;

	dsi_set_cmdq(data_array, 7, 0);

}
#endif


LCM_DRIVER hx8394f_boe_720p_vdo_drv = 
{
    .name			= "hx8394f_boe_720p_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.compare_id     = lcm_compare_id,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
#if defined(LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
    };
