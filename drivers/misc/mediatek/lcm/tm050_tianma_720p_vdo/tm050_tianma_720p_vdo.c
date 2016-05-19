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

extern int tps65132_write_bytes(unsigned char addr, unsigned char value);
#if 0
static int tps65132_write_bytes_1(unsigned char addr, unsigned char value)
{	
   

	int ret = 0;
	unsigned char xfers = 1;
	int retries = 3;//try 3 timers
     
	struct i2c_client *client = tps65132_i2c_client;
	unsigned char write_data[8];
	write_data[0]= addr;
	write_data[1]= value;
	//memcpy(&write_data[1], &value, 1);
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
			printk("yufangfang tps65132 write data fail !!\n");
			break;
		}
	} while (ret != xfers && --retries);

	return ret == xfers ? 1 : -1;
}
EXPORT_SYMBOL_GPL(tps65132_write_bytes_1);
#endif

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

	  {0xFF,3,{0x98,0x81,0x03}},
	{0x01,1,{0x00}}, 
  {0x02,1,{0x00}}, 
  {0x03,1,{0x73}}, 
  {0x04,1,{0xD3}}, 
  {0x05,1,{0x00}}, 
  {0x06,1,{0x0A}}, 
  {0x07,1,{0x0E}}, 
  {0x08,1,{0x00}}, 
  {0x09,1,{0x01}}, 
  {0x0A,1,{0x01}}, 
  {0x0B,1,{0x01}}, 
  {0x0C,1,{0x01}}, 
  {0x0D,1,{0x01}}, 
  {0x0E,1,{0x01}}, 
  {0x0F,1,{0x15}},	
  
  {0x10,1,{0x15}},
  {0x11,1,{0x00}},
  {0x12,1,{0x00}},
  {0x13,1,{0x00}},
  {0x14,1,{0x00}},
  {0x15,1,{0x08}},
  {0x16,1,{0x08}},
  {0x17,1,{0x08}},
  {0x18,1,{0x08}},
  {0x19,1,{0x00}},
  {0x1A,1,{0x00}},
  {0x1B,1,{0x00}},
  {0x1C,1,{0x00}},
  {0x1D,1,{0x00}},
  {0x1E,1,{0x40}},
  {0x1F,1,{0x80}},

  {0x20,1,{0x06}},
  {0x21,1,{0x01}},
  {0x22,1,{0x00}},
  {0x23,1,{0x00}},
  {0x24,1,{0x00}},
  {0x25,1,{0x00}},
  {0x26,1,{0x00}},
  {0x27,1,{0x00}},
  {0x28,1,{0x33}},
  {0x29,1,{0x03}},
  {0x2A,1,{0x00}},
  {0x2B,1,{0x00}},
  {0x2C,1,{0x00}},
  {0x2D,1,{0x00}},
  {0x2E,1,{0x00}},
  {0x2F,1,{0x00}},

  {0x30,1,{0x00}},
  {0x31,1,{0x00}},
  {0x32,1,{0x00}},
  {0x33,1,{0x00}},
  {0x34,1,{0x03}},
  {0x35,1,{0x00}},
  {0x36,1,{0x03}},
  {0x37,1,{0x00}},
  {0x38,1,{0x00}},
  {0x39,1,{0x00}},
  {0x3A,1,{0x40}},
  {0x3B,1,{0x40}},
  {0x3C,1,{0x00}},
  {0x3D,1,{0x00}},
  {0x3E,1,{0x00}},
  {0x3F,1,{0x00}},

  {0x40,1,{0x00}},
  {0x41,1,{0x00}},
  {0x42,1,{0x00}},
  {0x43,1,{0x00}},
  {0x44,1,{0x00}},

  {0x50,1,{0x01}},
  {0x51,1,{0x23}},
  {0x52,1,{0x45}},
  {0x53,1,{0x67}},
  {0x54,1,{0x89}},
  {0x55,1,{0xab}},
  {0x56,1,{0x01}},
  {0x57,1,{0x23}},
  {0x58,1,{0x45}},
  {0x59,1,{0x67}},
  {0x5a,1,{0x89}},
  {0x5b,1,{0xab}},
  {0x5c,1,{0xcd}},
  {0x5d,1,{0xef}},
  {0x5e,1,{0x11}},
  {0x5f,1,{0x0D}},

  {0x60,1,{0x0C}},
  {0x61,1,{0x0F}},
  {0x62,1,{0x0E}},
  {0x63,1,{0x06}},
  {0x64,1,{0x07}},
  {0x65,1,{0x02}},
  {0x66,1,{0x02}},
  {0x67,1,{0x02}},
  {0x68,1,{0x02}},
  {0x69,1,{0x02}},
  {0x6a,1,{0x02}},
  {0x6b,1,{0x02}},
  {0x6c,1,{0x02}},
  {0x6d,1,{0x02}},
  {0x6e,1,{0x02}},
  {0x6f,1,{0x02}},

  {0x70,1,{0x02}},
  {0x71,1,{0x02}},
  {0x72,1,{0x08}},
  {0x73,1,{0x00}},
  {0x74,1,{0x01}},
  {0x75,1,{0x0D}},
  {0x76,1,{0x0C}},
  {0x77,1,{0x0F}},
  {0x78,1,{0x0E}},
  {0x79,1,{0x06}},
  {0x7a,1,{0x07}},
  {0x7b,1,{0x02}},
  {0x7c,1,{0x02}},
  {0x7d,1,{0x02}},
  {0x7e,1,{0x02}},
  {0x7f,1,{0x02}},

  {0x80,1,{0x02}},
  {0x81,1,{0x02}},
  {0x82,1,{0x02}},
  {0x83,1,{0x02}},
  {0x84,1,{0x02}},
  {0x85,1,{0x02}},
  {0x86,1,{0x02}},
  {0x87,1,{0x02}},
  {0x88,1,{0x08}},
  {0x89,1,{0x00}},
  {0x8a,1,{0x01}},
  
  //-------------------Power setting---------------------------------
  {0xFF,3,{0x98,0x81,0x04}},  
    {0x6c,1,{0x15}},	  	// VCORE: 1.5V 	                              
    {0x6e,1,{0x3B}},		// VGH: 3B=18.6V 					
    {0x6f,1,{0x37}},		// VGH:3x VGL:-2x VCL:REG	
    {0x8d,1,{0x14}},		// VGL: -10V              
    {0x3a,1,{0x24}},		// Power saving	
    {0x35,1,{0x1F}},         
    {0x87,1,{0xBA}},   	//ESD Enhance           
                            
    {0x26,1,{0x76}},		//ESD Enhance             
    {0xb2,1,{0xd1}},		//ESD Enhance             
    {0xb5,1,{0x06}},		//gamma bias: medium high 
    {0x31,1,{0x75}},		//sd_sap[2:0]:111         
   // {0x63,1,{0xc0}},   	//LVD»úÖÆÆô¶¯
    
    //----------- Page 1 Command -------------           
    {0xFF,3,{0x98,0x81,0x01}},
   {0x22,1,{0x0A}},	   	// 0AÕýÉš 09·ŽÉš    
   {0x31,1,{0x00}},		// Inversion:00 Column
   {0x50,1,{0x8D}},		// VSP  4.4V          
   {0x51,1,{0x98}},		// VSN  -4.6V         
                         
   {0x60,1,{0x1F}},	   	// Source Timing    
   
   //-----------------------GAMMA SETTING------------------- 
   //------------------------P-tive setting----------------- 
  {0xA0,1,{0x1D}},		   //255               
  {0xA1,1,{0x30}},		   //251               
  {0xA2,1,{0x3C}},		   //247               
  {0xA3,1,{0x13}},		   //243               
  {0xA4,1,{0x15}},		   //239               
  {0xA5,1,{0x28}},		   //231               
  {0xA6,1,{0x1B}},		   //219               
  {0xA7,1,{0x1E}},		   //203               
  {0xA8,1,{0xA1}},		   //175               
  {0xA9,1,{0x1B}},		   //114               
  {0xAA,1,{0x29}},		   //111               
  {0xAB,1,{0x8A}},		   //80                
  {0xAC,1,{0x1E}},		   //52                
  {0xAD,1,{0x1F}},		   //36                
  {0xAE,1,{0x53}},		   //24                
  {0xAF,1,{0x27}},		   //16                
  {0xB0,1,{0x2C}},		   //12                
  {0xB1,1,{0x4C}},		   //8                 
  {0xB2,1,{0x56}},		   //4                 
  {0xB3,1,{0x1D}},		   //0					       


  {0xC0,1,{0x1D}},		   //255               
  {0xC1,1,{0x30}},		   //251               
  {0xC2,1,{0x3C}},		   //247               
  {0xC3,1,{0x13}},		   //243               
  {0xC4,1,{0x15}},		   //239               
  {0xC5,1,{0x28}},		   //231               
  {0xC6,1,{0x1B}},		   //219               
  {0xC7,1,{0x1E}},		   //203               
  {0xC8,1,{0xA1}},		   //175               
  {0xC9,1,{0x1B}},		   //114               
  {0xCA,1,{0x29}},		   //111               
  {0xCB,1,{0x8A}},		   //80                
  {0xCC,1,{0x1E}},		   //52                
  {0xCD,1,{0x1F}},		   //36                
  {0xCE,1,{0x53}},		   //24                
  {0xCF,1,{0x27}},		   //16                
  {0xD0,1,{0x2C}},		   //12                
  {0xD1,1,{0x4C}},		   //8                 
  {0xD2,1,{0x56}},		   //4                 
  {0xD3,1,{0x1D}},		   //0						   	 

   {0xFF,3,{0x98,0x81,0x00}},	                                     
   {0x51,2,{0x0F,0xFF}},       
   {0x53,1,{0x24}},            
   {0x55,1,{0x00}},            
   {0x35,1,{0x00}},            	

    { 0x11, 0, {} },
    {REGFLAG_DELAY, 120, {}},
    { 0x29, 0, {} },
    {REGFLAG_DELAY, 20, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

void push_table_1(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
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

#ifdef BUILD_LK
static struct mt_i2c_t ili9881_i2c;
#define ILI9881_SLAVE_ADDR 0x3e
/**********************************************************
  *
  *   [I2C Function For Read/Write ili9881] 
  *
  *********************************************************/
static unsigned int ili9881_write_byte(unsigned char addr, unsigned char value)
{
    unsigned int ret_code = I2C_OK;
    unsigned char write_data[2];
    kal_uint16 len;

    write_data[0]= addr;
    write_data[1] = value;

    ili9881_i2c.id = 2;
    /* Since i2c will left shift 1 bit, we need to set ili9881 I2C address to >>1 */
    ili9881_i2c.addr = (ILI9881_SLAVE_ADDR);
    ili9881_i2c.mode = ST_MODE;
    ili9881_i2c.speed = 100;
    len = 2;

    ret_code = i2c_write(&ili9881_i2c, write_data, len);
    printf("%s: i2c_write: ret_code: %d\n", __func__, ret_code);

    return ret_code;
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
	params->physical_width = 62;
	params->physical_height = 111;
	// enable tearing-free
	//params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
	//params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
#ifdef CONFIG_SLT_DRV_DEVINFO_SUPPORT
	params->module="TM050JDSP09";
	params->vendor="tianma";
	params->ic="ili9881c";
	params->info="720*1280";
#endif

#if defined(LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
#else
	params->dsi.mode   = BURST_VDO_MODE;//SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;
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

	//params->dsi.DSI_WMEM_CONTI=0x3C; 
	//params->dsi.DSI_RMEM_CONTI=0x3E; 


	params->dsi.packet_size = 256;

	/* Video mode setting */
	//params->dsi.intermediat_buffer_num = 2;

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active				= 8; //10
	params->dsi.vertical_backporch					= 20; //20
	params->dsi.vertical_frontporch					= 60; //24
	params->dsi.vertical_active_line				= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active				= 20; //32
	params->dsi.horizontal_backporch				= 160;
	params->dsi.horizontal_frontporch				= 120; //60
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;


	params->dsi.HS_TRAIL = 15;

	// Bit rate calculation
	params->dsi.PLL_CLOCK				= 255;//
	//esd check 0x09->0x80 0x73 0x04     0xd9->0x80
	params->dsi.clk_lp_per_line_enable = 1;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
}
//#ifdef BUILD_LK
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
		   //printf("[LK]---cmd---ili9881----%s------\n",__func__);
		   ili9881_write_byte(0x00, 0x0f);
		   ili9881_write_byte(0x01, 0x0f);
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
//#endif
#define TM050_LCM_ID (0x9881)
//#define LCM_ID (GPIO0 | 0x80000000)

static unsigned int lcm_compare_id(void)
{
		int array[4];
		char buffer[3];
		unsigned int id0,id1,id=0;
	
        KTD2125_enable(1);

        SET_RESET_PIN(1);
        MDELAY(20);
        SET_RESET_PIN(0);
        MDELAY(10);
        SET_RESET_PIN(1);
	    MDELAY(120);

        array[0] = 0x00043902; 						 
	    array[1] = 0x018198ff; 				
	    dsi_set_cmdq(array, 2, 1); 



		array[0] = 0x00013700;// read id return two byte,version and id
		dsi_set_cmdq(array, 1, 1);
		read_reg_v2(0x00, buffer, 1);
		id0  =  buffer[0]; 

		array[0] = 0x00013700;// read id return two byte,version and id
		dsi_set_cmdq(array, 1, 1);
		read_reg_v2(0x01, buffer, 1);
		id1  =  buffer[0]; 
		    KTD2125_enable(0);
		    id = (id0 << 8) | id1;
#ifdef BUILD_LK
	printf("yufangfang tm050_tianma %s, id0 = 0x%08x , id1 = 0x%08x, id = 0x%08x\n", __func__, id0, id1, id);
#else
	printk("yufangfang tm050_tianma %s, id0 = 0x%08x , id1 = 0x%08x, id = 0x%08x\n", __func__, id0, id1, id);
#endif

        if ( (id == TM050_LCM_ID)) //cpt
        {
	   return 1;
        }
        else
           return 0;


}

static void lcm_init(void)
{

    #ifdef BUILD_LK
		  printf("[LK]---cmd---ili9881----%s------\n",__func__);
    #else
		  printk("[KERNEL]---cmd---ili9881-----%s------\n",__func__);
    #endif
    KTD2125_enable(1);
    SET_RESET_PIN(1);
    MDELAY(20);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);

    push_table_1(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}



static void lcm_suspend(void)
{

	unsigned int data_array[2];



	data_array[0] = 0x00043902; 						 
	data_array[1] = 0x008198ff; 				
	dsi_set_cmdq(data_array, 2, 1); 
	MDELAY(1);

	data_array[0]=0x00280500; // Display Off
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);
	data_array[0] = 0x00100500; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);

        KTD2125_enable(0);

#ifdef BUILD_LK
	printf("uboot %s\n", __func__);
#else
	pr_debug("kernel %s\n", __func__);
#endif

}


static void lcm_resume(void)
{
/*
	unsigned int data_array[2];
        KTD2125_enable(1);
	data_array[0] = 0x00110500; // Display Off
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120); 
	data_array[0] = 0x00290500; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);
*/
#ifdef BUILD_LK
	printf("uboot %s\n", __func__);
#else
	pr_debug("kernel %s\n", __func__);
#endif

//	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
	lcm_init();
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
	pr_debug("kernel %s\n", __func__);
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


LCM_DRIVER tm050_tianma_720p_vdo_drv = 
{
    .name			= "tm050_tianma_720p_vdo",
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
