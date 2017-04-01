//liujinzhou@wind-mobi.com 20161202 begin
/* BEGIN PN: , Added by guohongjin, 2014.08.13*/


//#ifdef BUILD_LK
//	#include <platform/mt_gpio.h>
//#elif defined(BUILD_UBOOT)
//	#include <asm/arch/mt_gpio.h>
//#else
//	#include <mach/mt_gpio.h>
//#endif

#ifdef BUILD_LK
#include <platform/gpio_const.h>
#include <platform/mt_gpio.h>
#include <platform/upmu_common.h>
#else
    #include <linux/string.h>
    #if defined(BUILD_UBOOT)
        #include <asm/arch/mt_gpio.h>
    #else
//sunsiyuan@wind-mobi.com 20160408 begin
		#include <mt-plat/mt_gpio.h>
		#include <mach/gpio_const.h>
//sunsiyuan@wind-mobi.com 20160408 end
    #endif
#endif


#include "lcm_drv.h"


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define LCM_DSI_CMD_MODE									0
#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)
//liujinzhou@wind-mobi.com modify at 20161202 begin
#define LCM_ID_HX8394                                                              (0x1a50)//0x8394->0x1a50
//liujinzhou@wind-mobi.com modify at 20161202 end

#define REGFLAG_DELAY             								0xFC
#define REGFLAG_END_OF_TABLE      							0xFD   // END OF REGISTERS MARKER

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

//sunsiyuan@wind-mobi.com modify ata_check at 20170210 begin
#ifdef CONFIG_WIND_DEVICE_INFO
               extern char *g_lcm_name;
#endif
//sunsiyuan@wind-mobi.com modify ata_check at 20170210 end

//liujinzhou@wind-mobi.com at 20161201 begin
#define GPIO_LCD_BIAS_ENP_PIN         (GPIO78 | 0x80000000)
//liujinzhou@wind-mobi.com at 20161201 end
static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util;

#define __SAME_IC_COMPATIBLE__

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))
#define MDELAY(n) 											(lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

//liupeng@wind-mobi.com 20160315 begin
//sunsiyuan@wind-mobi.com at 20160408 begin
#if 0
static struct platform_device * pltfm_dev ;

struct pinctrl *lcmbiasctrl = NULL;
struct pinctrl_state *lcmbias_enable= NULL;
struct pinctrl_state *lcmbias_disable= NULL;


static int lcmbias_probe(struct platform_device *dev)
{
	printk("[lcm]lcmbias_probe begin!\n");
	pltfm_dev = dev;
	
		lcmbiasctrl = devm_pinctrl_get(&pltfm_dev->dev);
	if (IS_ERR(lcmbiasctrl)) {
		dev_err(&pltfm_dev->dev, "Cannot find  lcmbias pinctrl!");
	}
    /*Cam0 Power/Rst Ping initialization*/
	lcmbias_enable = pinctrl_lookup_state(lcmbiasctrl, "state_enable_output1");
	if (IS_ERR(lcmbias_enable)) {
		pr_debug("%s : pinctrl err, lcmbias_enable\n", __func__);
	}

	lcmbias_disable = pinctrl_lookup_state(lcmbiasctrl, "state_enable_output0");
	if (IS_ERR(lcmbias_disable)) {
		pr_debug("%s : pinctrl err, lcmbias_disable\n", __func__);
	}
	printk("[lcm]lcmbias_probe done!\n");
	return 0;
}

static int lcmbias_remove(struct platform_device *dev)
{

	return 0;
}


struct of_device_id lcmbias_of_match[] = {
	{ .compatible = "mediatek,lcmbias", },
	{},
};

static struct platform_driver lcmbias_driver = {
	.probe = lcmbias_probe,
	.remove = lcmbias_remove,
	.driver = {
			.name = "lcmbias_drv",
			.of_match_table = lcmbias_of_match,
		   },
};
static int lcmbias_mod_init(void)
{
	int ret = 0;

	printk("[lcmbias]lcmbias_mod_init begin!\n");
	ret = platform_driver_register(&lcmbias_driver);
	if (ret)
		printk("[lcmbias]platform_driver_register error:(%d)\n", ret);
	else
		printk("[lcmbias]platform_driver_register done!\n");

		printk("[lcmbias]lcmbias_mod_init done!\n");
	return ret;

}

static void lcmbias_mod_exit(void)
{
	printk("[lcmdet]lcmbias_mod_exit\n");
	platform_driver_unregister(&lcmbias_driver);
	printk("[lcmdet]lcmbias_mod_exit Done!\n");
}

module_init(lcmbias_mod_init);
module_exit(lcmbias_mod_exit);
#endif
//sunsiyuan@wind-mobi.com at 20160408 end
//liupeng@wind-mobi.com 20160315 end
 struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};
//tuwenzan@wind-mobi.com modify at 20170402 beign
static struct LCM_setting_table lcm_initialization_setting1[] = 
{
		//liujinzhou@wind-mobi.com modify at 20170103 begin
	{0xB9, 3,{0xFF,0x83,0x94}},
			  
	{0xBA, 6,{0x63,0x03,0x68,0x6B,0xB2,0xC0}},
			  
	//{0xB1,1,{0x60}},
	
	{0xCC,1,{0x0B}},
			  
	{0xE0,58,{0x00,0x03,0x0A,0x10,0x11,0x15,0x19,0x15,0x2F,0x3F,0x4F,0x4F,0x5D,0x70,0x78,0x7E,0x8F,0x97,0x95,0xA2,0xB2,0x58,0x57,0x5D,0x63,0x67,0x6A,0x79,0x7F,0x00,0x03,0x0A,0x10,0x11,0x15,0x19,0x15,0x2F,0x3F,0x4F,0x4F,0x5D,0x70,0x78,0x7E,0x8F,0x97,0x95,0xA2,0xB2,0x58,0x57,0x5D,0x63,0x67,0x6A,0x79,0x7F}},		
	
    {0xD2,1,{66}},
	
	{0xB2, 6,{0x00,0x80,0x64,0x0C,0x06,0x2F}},
	//{0xB2, 6,{0x00,0x80,0x64,0x0e,0x06,0x2F}},
			  
	//{0xB4,21,{0x20,0x7C,0x20,0x7C,0x20,0x7C,0x01,0x05,0x86,0x35,0x00,0x3F,0x20,0x7C,0x20,0x7C,0x20,0x7C,0x01,0x05,0x86}},
	{0xB4,22,{0x20,0x7C,0x20,0x7C,0x20,0x7C,0x01,0x05,0xFF,0x35,0x00,0x3F,0x20,0x7C,0x20,0x7C,0x20,0x7c,0x01,0x05,0xFF,0x3F}},
			
    {0xD3,32,{0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x10,0x32,0x10,0x01,0x00,0x01,0x32,0x13,0xC1,0x00,0x01,0x32,0x10,0x08,0x00,0x00,0x37,0x03,0x03,0x37,0x05,0x05,0x37,0x0C,0x40}},
	
    {0xD5,44,{0x18,0x18,0x18,0x18,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x19,0x19,0x19,0x19,0x20,0x21,0x22,0x23}},
	
	//{0xB6, 2,{0x58,0x58}},		  
	{0xD6,44,{0x18,0x18,0x19,0x19,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x19,0x19,0x18,0x18,0x23,0x22,0x21,0x20}},
		
    {0xC0,2,{0x1F,0x73}},
	
	{0xD4,1,{0x02}},
	
	//{0xBD,1,{0x01}},
	
	//{0xB1,1,{0x60}},
	
	//{0xBD,1,{0x00}},
	
	{0xBD,1,{0x00}},
	
	{0xC1,43,{0x01,0x00,0x08,0x10,0x18,0x21,0x2A,0x31,0x39,0x40,0x46,0x4D,0x55,0x5C,0x64,0x6B,0x73,0x7A,0x82,0x8A,0x92,0x9B,0xA3,0xAC,0xB5,0xBC,0xC5,0xCD,0xD5,0xDE,0xE7,0xEF,0xF8,0xFF,0x1B,0x09,0x64,0x40,0x5B,0xB8,0x8B,0xA8,0xC0}},
	
	{0xBD,1,{0x01}},
	
	{0xC1,42,{0x00,0x08,0x10,0x19,0x21,0x2A,0x31,0x38,0x3F,0x45,0x4B,0x52,0x59,0x61,0x68,0x6F,0x76,0x7D,0x85,0x8D,0x95,0x9D,0xA4,0xAD,0xB5,0xBC,0xC4,0xCC,0xD4,0xDC,0xE4,0xED,0xF5,0x2C,0x87,0xAB,0xC1,0x34,0x1D,0x11,0x1C,0x40}},
	
	{0xBD,1,{0x02}},
	
	{0xC1,42,{0x00,0x07,0x0F,0x17,0x1E,0x27,0x2F,0x36,0x3D,0x43,0x49,0x50,0x56,0x5E,0x64,0x6C,0x72,0x7A,0x81,0x88,0x90,0x98,0xA0,0xA7,0xAF,0xB7,0xBE,0xC5,0xCD,0xD5,0xDC,0xE4,0xF4,0x34,0xA5,0x54,0xCC,0xC7,0x73,0xD7,0x8A,0x80}},
	
	{0xBD, 1,{0x00}},	
			  
	//{0x36, 1, {0x2}},
			  
	{0x11, 1,{0x00}},
	
	{REGFLAG_DELAY, 120, {0}},
	
	{0xBD,1,{0x01}},
	
	{0xB1,1,{0x00}},
	
	{0xBD,1,{0x00}},
	//{0x35,1,{0x00}},
	//{0xBE,2,{0x01,0x73}},
	//{0xD9,1,{0xBF}},
	//liupeng@wind-mobi.com 20160322 begin for ESD blurred screen
	{0xB2, 12,{0x00,0x80,0x64,0x0e,0x0a,0x2F,0x00,0x00,0x00,0x00,0xc0,0x18}},//liupeng	
	//liupeng@wind-mobi.com 20160322 end	  
	//{0xCC,1,{0x0f}},
	{0x29, 1,{0x00}},
	{REGFLAG_DELAY, 20, {0}},
	
	{REGFLAG_END_OF_TABLE, 0x00, {}}
	//liujinzhou@wind-mobi.com modify at 20170103 end
};
//tuwenzan@wind-mobi.com modify at 20170402 end
#if 0
static struct LCM_setting_table lcm_sleep_out_setting[] = {
    //Sleep Out
    {0x11, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
    {0x29, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};
#endif
static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
    // Display off sequence
    // Sleep Mode On
	 {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 50, {}},
	
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
	//zhangaifeng@wind-mobi.com begin
   // {0xFF,	3,		{0x98,0x81,0x01}},
   // {0x58, 1, {0x01}},
//{REGFLAG_DELAY, 20, {}},
		//zhangaifeng@wind-mobi.com end
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++)
    {
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

		params->dbi.te_mode				= LCM_DBI_TE_MODE_VSYNC_ONLY;
		//LCM_DBI_TE_MODE_DISABLED;
		//LCM_DBI_TE_MODE_VSYNC_ONLY;  
		params->dbi.te_edge_polarity		= LCM_POLARITY_RISING; 
		/////////////////////   
		//if(params->dsi.lcm_int_te_monitor)  
		//params->dsi.vertical_frontporch *=2;  
		//params->dsi.lcm_ext_te_monitor= 0;//TRUE; 
	//	params->dsi.noncont_clock= TRUE;//FALSE;   
	//	params->dsi.noncont_clock_period=2;
//		params->dsi.cont_clock=1;
		////////////////////          
		params->dsi.mode   = SYNC_PULSE_VDO_MODE;  
		// DSI    /* Command mode setting */  
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;      
		//The following defined the fomat for data coming from LCD engine.  
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;   
		params->dsi.data_format.trans_seq	= LCM_DSI_TRANS_SEQ_MSB_FIRST; 
		params->dsi.data_format.padding 	= LCM_DSI_PADDING_ON_LSB;    
		params->dsi.data_format.format	  = LCM_DSI_FORMAT_RGB888;       
		// Video mode setting		   
		params->dsi.intermediat_buffer_num = 2;  
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;  
		params->dsi.packet_size=256;    
		// params->dsi.word_count=480*3;	
		//DSI CMD mode need set these two bellow params, different to 6577   
		// params->dsi.vertical_active_line=800;   
		params->dsi.vertical_sync_active				= 4; //4   
		params->dsi.vertical_backporch				       = 12;  //14  
		params->dsi.vertical_frontporch				       = 10;  //16  
		params->dsi.vertical_active_line				       = FRAME_HEIGHT;     
		params->dsi.horizontal_sync_active				= 80;   //30
		params->dsi.horizontal_backporch				= 95;  //80  
		params->dsi.horizontal_frontporch				= 95;    //80
//		params->dsi.horizontal_blanking_pixel				= 60;   
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;  

	//	params->dsi.HS_TRAIL=14;
	//	params->dsi.pll_div1=1;		   
	//	params->dsi.pll_div2=1;		   
	//	params->dsi.fbk_div =28;//28	
//zhounengwen@wind-mobi.com 20150327 beign
// To fix lcm rf
        params->dsi.PLL_CLOCK = 232;

//		params->dsi.CLK_TRAIL = 17;
	    //liujinzhou@wind-mobi.com modify at 20161205 begin
	    params->dsi.cont_clock=0;   
		#if 1 //add by liupeng@wind-mobi.com 20160125 begin
	    params->dsi.esd_check_enable = 1;
	    params->dsi.customization_esd_check_enable      = 1;
		//liujinzhou@wind-mobi.com modify at 20161205 end

	    params->dsi.lcm_esd_check_table[0].cmd          = 0xd9;
	    params->dsi.lcm_esd_check_table[0].count        = 1;
	    params->dsi.lcm_esd_check_table[0].para_list[0] = 0x80;

	    params->dsi.lcm_esd_check_table[1].cmd          = 0x09;
	    params->dsi.lcm_esd_check_table[1].count        = 3;
	    params->dsi.lcm_esd_check_table[1].para_list[0] = 0x80;
	    params->dsi.lcm_esd_check_table[1].para_list[1] = 0x73;
	    params->dsi.lcm_esd_check_table[1].para_list[2] = 0x04;
		//liujinzhou@wind-mobi.com modify at 20121205 begin
		params->dsi.lcm_esd_check_table[2].cmd          = 0x45;
        params->dsi.lcm_esd_check_table[2].count        = 2;
		params->dsi.lcm_esd_check_table[2].para_list[0] = 0x05;
		params->dsi.lcm_esd_check_table[2].para_list[1] = 0x18;
		//liujinzhou@wind-mobi.com modify at 20121205 end
		#endif //add by liupeng@wind-mobi.com 20160125 end
		//tuwenzan@wind-mobi.com add physical size at 20170217 begin
		params->physical_width  = 118;
		params->physical_height = 65;
		//tuwenzan@wind-mobi.com add physical size at 20170217 begin

}

static void lcm_init(void)
{
//liupeng@wind-mobi.com 20150315 begin
//sunsiyuan@wind-mobi.com at 20160408 begin
#ifdef GPIO_LCD_BIAS_ENP_PIN
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
#endif
//pinctrl_select_state(lcmbiasctrl, lcmbias_enable); 
//sunsiyuan@wind-mobi.com at 20160408 end
 //liupeng@wind-mobi.com 20150315 end
	MDELAY(10);

	SET_RESET_PIN(1);
	MDELAY(5);////huyunge@wind-mobi.com modify delay 120ms==>55ms for lcd out sleep time.
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(55); //huyunge@wind-mobi.com modify delay 120ms==>55ms for lcd out sleep time.
//	printk(" gemingming hx8394f init  \n");
	push_table(lcm_initialization_setting1, sizeof(lcm_initialization_setting1) / sizeof(struct LCM_setting_table), 1); 

		  
}

static void lcm_suspend(void) 
{



	// when phone sleep , config output low, disable backlight drv chip  
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
		MDELAY(10);
//liupeng@wind-mobi.com 20160315 begin
//sunsiyuan@wind-mobi.com at 20160408 begin
#ifdef GPIO_LCD_BIAS_ENP_PIN
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);
#endif
//pinctrl_select_state(lcmbiasctrl, lcmbias_disable); 
//sunsiyuan@wind-mobi.com at 20160408 end
//liupeng@wind-mobi.com 20160315 end
	MDELAY(10);

}

static void lcm_resume(void)
{
	lcm_init();

}

static unsigned int lcm_compare_id(void)
{
	unsigned int id=0,id1=0,id2=0;
	unsigned char buffer[3];
	unsigned int data_array[16];  
//liupeng@wind-mobi.com 20160315 begin
//sunsiyuan@wind-mobi.com at 20160408 begin
#ifdef GPIO_LCD_BIAS_ENP_PIN
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
#endif
//pinctrl_select_state(lcmbiasctrl, lcmbias_enable); 
//sunsiyuan@wind-mobi.com at 20160408 end
//liupeng@wind-mobi.com 20160315 end
	SET_RESET_PIN(1);  //NOTE:should reset LCM firstly
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(120); 

	data_array[0]=0x00043902;
	data_array[1]=0x9483FFB9;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(10);

	data_array[0]=0x00023902;
	data_array[1]=0x000013ba;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(10);

	data_array[0] = 0x00023700;// return byte number
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10);

	read_reg_v2(0xDA, buffer, 1);
	id1= buffer[0]; //should be 0x83
	read_reg_v2(0xDB, buffer, 1);
	id2= buffer[0]; //should be 0x94

	id=(id1 << 8) | id2;

#if defined(BUILD_LK)||defined(BUILD_UBOOT)
	printf(" hzs %s id=%x id1=%x id2=%x \n",__func__,id, id1, id2);
#else
	printk("dzl------------- %s id=%x  \n",__func__,id);
#endif	

	if(LCM_ID_HX8394==id)
	{
		return 1;
    }
		
	else
	{
	return 0;
	}
	
}
//static int err_count = 0;
static unsigned int lcm_esd_check(void)
{
  #ifndef BUILD_LK
    unsigned char buffer[8] = {0};

    unsigned int array[4];

	if(lcm_esd_test)
	{
		lcm_esd_test = FALSE;
		return TRUE;
	}

    array[0] = 0x00013700;    

    dsi_set_cmdq(array, 1,1);

    read_reg_v2(0x0A, buffer,8);

	printk( "ili9881 lcm_esd_check: buffer[0] = %d,buffer[1] = %d,buffer[2] = %d,buffer[3] = %d,buffer[4] = %d,buffer[5] = %d,buffer[6] = %d,buffer[7] = %d\n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7]);

    if((buffer[0] != 0x9C))/*LCD work status error,need re-initalize*/

    {

        printk( "ili9881 lcm_esd_check buffer[0] = %d\n",buffer[0]);

        return TRUE;

    }

    else

    {

#if 0
        if(buffer[3] != 0x02) //error data type is 0x02

        {
		//  is not 02, 
             err_count = 0;

        }

        else

        {
		// is 02, so ,
             if((buffer[4] == 0x40) || (buffer[5] == 0x80))
             {
			 // buffer[4] is not 0, || (huo),buffer[5] is 0x80.
			   err_count = 0;
             }
             else

             {
			// is  0,0x80,  
			   err_count++;
             }             

             if(err_count >=2 )
             {
			
                 err_count = 0;

                 printk( "ili9881 lcm_esd_check buffer[4] = %d , buffer[5] = %d\n",buffer[4],buffer[5]);

                 return TRUE;

             }

        }
#endif
        return FALSE;

    }
#endif
	
}
static unsigned int lcm_esd_recover(void)
{
    #ifndef BUILD_LK
    printk( "ili9881 lcm_esd_recover\n");
    #endif
	lcm_init();
//	lcm_resume();

	return TRUE;
}

//sunsiyuan@wind-mobi.com modify ata_check at 20170210 begin
static unsigned int lcm_ata_check(unsigned char *buf)
{
       #ifdef CONFIG_WIND_DEVICE_INFO
       if(!strcmp(g_lcm_name,"hx8394f_hd720_dsi_vdo_boe")){
               return 1;
       }else{
               return -1;
       }
       #endif
}
//sunsiyuan@wind-mobi.com modify ata_check at 20170210 end

#ifdef WIND_LCD_POWER_SUPPLY_SUPPORT
extern void lcm_init_power(void);
extern void lcm_resume_power(void);
extern void lcm_suspend_power(void);
#endif

LCM_DRIVER hx8394f_hd720_dsi_vdo_boe_lcm_drv =
{
	.name           	= "hx8394f_hd720_dsi_vdo_boe",
	.set_util_funcs 	= lcm_set_util_funcs,
	.get_params     	= lcm_get_params,
	.init           	= lcm_init,
	.suspend        	= lcm_suspend,
	.resume         	= lcm_resume,
	.compare_id     	= lcm_compare_id,
	.esd_check = lcm_esd_check,
	.esd_recover = lcm_esd_recover,
	.ata_check          = lcm_ata_check,    //sunsiyuan@wind-mobi.com add ata_check at 20170210

#ifdef WIND_LCD_POWER_SUPPLY_SUPPORT
	.init_power		= lcm_init_power,
	.resume_power   = lcm_resume_power,
	.suspend_power  = lcm_suspend_power,
#endif
};
//late_initcall(lcm_init);
/* END PN: , Added by guohongjin, 2014.08.13*/
//liujinzhou@wind-mobi.com modify at 20161202 end
