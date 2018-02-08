//qiumeng@wind-mobi.com add at 20170425 begin

#include "lcm_drv.h"
/***********add for extern avdd ---shenyong@wind-mobi.com---start*********/
#include "lcm_extern.h"
/***********add for extern avdd ---shenyong@wind-mobi.com---end*********/
#ifdef BUILD_LK
#include <platform/gpio_const.h>
#include <platform/mt_gpio.h>
#include <platform/upmu_common.h>
#else
#include <linux/string.h>
#if defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
	#include <mt-plat/mt_gpio.h>
	#include <mach/gpio_const.h>
#endif
#endif

//liujinzhou@wind-mobi.com add at 20161231 begin
#ifdef CONFIG_WIND_DEVICE_INFO
		extern char *g_lcm_name;
#endif
//liujinzhou@wind-mobi.com add at 20161231 end
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define LCM_DSI_CMD_MODE									0
#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)
#define LCM_ID_ILI9881                                                              (0x9800)    

#define REGFLAG_DELAY             								0xFC
#define REGFLAG_END_OF_TABLE      							0xFD   // END OF REGISTERS MARKER

#define GPIO_LCD_BIAS_ENP_PIN         (GPIO119 | 0x80000000)

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

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


 struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {	
//config ASG timing
   {0xFF,3,{0x98,0x81,0x03}},
   {0x01,1,{0x08}},
   {0x02,1,{0x00}},
   {0x03,1,{0x73}},
   {0x04,1,{0x73}},
   {0x05,1,{0x14}},
   {0x06,1,{0x06}},
   {0x07,1,{0x02}},
   {0x08,1,{0x05}},
   {0x09,1,{0x00}},
   {0x0A,1,{0x0C}},
   {0x0B,1,{0x00}},
   {0x0C,1,{0x1C}},
   {0x0D,1,{0x1C}},
   {0x0E,1,{0x00}},
   {0x0F,1,{0x0C}},

   {0x10,1,{0x0C}},
   {0x11,1,{0x01}},
   {0x12,1,{0x01}},
   {0x13,1,{0x1B}},
   {0x14,1,{0x0B}},
   {0x15,1,{0x00}},
   {0x16,1,{0x00}},
   {0x17,1,{0x00}},
   {0x18,1,{0x00}},
   {0x19,1,{0x00}},
   {0x1A,1,{0x00}},
   {0x1B,1,{0x00}},
   {0x1C,1,{0x00}},
   {0x1D,1,{0x00}},
   {0x1E,1,{0xC8}},
   {0x1F,1,{0x80}},

   {0x20,1,{0x02}},
   {0x21,1,{0x00}},
   {0x22,1,{0x02}},
   {0x23,1,{0x00}},
   {0x24,1,{0x00}},
   {0x25,1,{0x00}},
   {0x26,1,{0x00}},
   {0x27,1,{0x00}},
   {0x28,1,{0xFB}},
   {0x29,1,{0x43}},
   {0x2A,1,{0x00}},
   {0x2B,1,{0x00}},
   {0x2C,1,{0x07}},
   {0x2D,1,{0x07}},
   {0x2E,1,{0xFF}},
   {0x2F,1,{0xFF}},

   {0x30,1,{0x11}},
   {0x31,1,{0x00}},
   {0x32,1,{0x00}},
   {0x33,1,{0x00}},
   {0x34,1,{0x84}},
   {0x35,1,{0x80}},
   {0x36,1,{0x07}},
   {0x37,1,{0x00}},
   {0x38,1,{0x00}},
   {0x39,1,{0x00}},
   {0x3A,1,{0x00}},
   {0x3B,1,{0x00}},
   {0x3C,1,{0x00}},
   {0x3D,1,{0x00}},
   {0x3E,1,{0x00}},
   {0x3F,1,{0x00}},

   {0x40,1,{0x00}},
   {0x41,1,{0x88}},
   {0x42,1,{0x00}},
   {0x43,1,{0x80}},
   {0x44,1,{0x08}},
       
   {0x50,1,{0x01}},
   {0x51,1,{0x23}},
   {0x52,1,{0x45}},
   {0x53,1,{0x67}},
   {0x54,1,{0x89}},
   {0x55,1,{0xAB}},
   {0x56,1,{0x01}},
   {0x57,1,{0x23}},
   {0x58,1,{0x45}},
   {0x59,1,{0x67}},
   {0x5A,1,{0x89}},
   {0x5B,1,{0xAB}},
   {0x5C,1,{0xCD}},
   {0x5D,1,{0xEF}},      
   {0x5E,1,{0x10}},
   {0x5F,1,{0x02}},

   {0x60,1,{0x02}},
   {0x61,1,{0x02}},
   {0x62,1,{0x02}},
   {0x63,1,{0x02}},
   {0x64,1,{0x02}},
   {0x65,1,{0x02}},
   {0x66,1,{0x08}},
   {0x67,1,{0x09}},
   {0x68,1,{0x02}},
   {0x69,1,{0x10}},
   {0x6A,1,{0x12}},
   {0x6B,1,{0x11}},
   {0x6C,1,{0x13}},
   {0x6D,1,{0x0C}},
   {0x6E,1,{0x0E}},
   {0x6F,1,{0x0D}},

   {0x70,1,{0x0F}},
   {0x71,1,{0x06}},
   {0x72,1,{0x07}},
   {0x73,1,{0x02}},
   {0x74,1,{0x02}},
   {0x75,1,{0x02}},
   {0x76,1,{0x02}},
   {0x77,1,{0x02}},
   {0x78,1,{0x02}},
   {0x79,1,{0x02}},
   {0x7A,1,{0x02}},
   {0x7B,1,{0x02}},
   {0x7C,1,{0x07}},
   {0x7D,1,{0x06}},
   {0x7E,1,{0x02}},
   {0x7F,1,{0x11}},

   {0x80,1,{0x13}},
   {0x81,1,{0x10}},
   {0x82,1,{0x12}},
   {0x83,1,{0x0F}},
   {0x84,1,{0x0D}},
   {0x85,1,{0x0E}},
   {0x86,1,{0x0C}},
   {0x87,1,{0x09}},
   {0x88,1,{0x08}},
   {0x89,1,{0x02}},
   {0x8A,1,{0x02}},

//Power setting
   {0xFF,3,{0x98,0x81,0x04}},
   //qiumeng@wind-mobi.com 20170522 begin
   //{0x00,1,{0x80}},             //0x80for4lane,0x00for3lane 
   {0x6C,1,{0x15}},             // VCORE: 1.5V 
   {0x6E,1,{0x2B}},             // VGH: 2B=15V 
   {0x6F,1,{0x45}},             // VGH:3x VGL:-2.5x VCL:REG
   //qiumeng@wind-mobi.com 20170522 end
   {0x3A,1,{0x24}},             // Power saving
   {0x8D,1,{0x1F}},             // VGL: -11V
   {0x87,1,{0xBA}},             //ESD Enhance
   {0x26,1,{0x76}},             //ESD Enhance
   {0xB2,1,{0xD1}},             //ESD Enhance
   {0x88,1,{0x0B}},
   {0x69,1,{0x57}},
   {0x33,1,{0x44}},
   {0x35,1,{0x1F}},
   {0xB5,1,{0x07}},             //gamma bias: medium high
   
//CMD_Page 1
   {0xFF,3,{0x98,0x81,0x01}},
   {0x22,1,{0x0A}},             // 0AÕýÉ¨ 09·´É¨
   {0x31,1,{0x0B}},             // Inversion:00 Column
   //qiumeng@wind-mobi.com 20170522 begin
   //{0x53,1,{0x60}},             //vcom1 
   //{0x55,1,{0x60}},             //vcom2
   {0x50,1,{0x85}},             // VSP  5V
   {0x51,1,{0x80}},             // VSN  -5V
   //qiumeng@wind-mobi.com 20170522 end
   {0x60,1,{0x0A}},             // Source Timing

   
//------------------GAMMA SETTING------------------
//qiumeng@wind-mobi.com 20170522 begin
//P-tive setting
   {0xA0,1,{0x00}},				//VP255	Gamma P
   {0xA1,1,{0x0B}},				//VP251  
   {0xA2,1,{0x18}},				//VP247  
   {0xA3,1,{0x10}},				//VP243  
   {0xA4,1,{0x12}},             //VP239  
   {0xA5,1,{0x22}},             //VP231  
   {0xA6,1,{0x15}},             //VP219  
   {0xA7,1,{0x19}},             //VP203 
   {0xA8,1,{0x64}},             //VP175  
   {0xA9,1,{0x1A}},             //VP114  
   {0xAA,1,{0x28}},             //VP111
   {0xAB,1,{0x63}},             //VP80   
   {0xAC,1,{0x18}},             //VP52 
   {0xAD,1,{0x13}},             //VP36
   {0xAE,1,{0x48}},             //VP24   
   {0xAF,1,{0x21}},             //VP16   
   {0xB0,1,{0x29}},             //VP12
   {0xB1,1,{0x56}},             //VP8
   {0xB2,1,{0x6A}},             //VP4
   {0xB3,1,{0x3D}},             //VP0

//N-tive setting
   {0xC0,1,{0x00}},			  	//VN255 GAMMA N
   {0xC1,1,{0x0D}},             //VN251   
   {0xC2,1,{0x16}},             //VN247   
   {0xC3,1,{0x0F}},             //VN243 
   {0xC4,1,{0x0F}},             //VN239   
   {0xC5,1,{0x22}},             //VN231  
   {0xC6,1,{0x17}},             //VN219  
   {0xC7,1,{0x18}},             //VN203 
   {0xC8,1,{0x65}},             //VN175    
   {0xC9,1,{0x1D}},             //VN144 
   {0xCA,1,{0x28}},             //VN111   
   {0xCB,1,{0x69}},             //VN80   
   {0xCC,1,{0x1F}},             //VN52  
   {0xCD,1,{0x20}},             //VN36    
   {0xCE,1,{0x54}},             //VN24     
   {0xCF,1,{0x23}},             //VN16   
   {0xD0,1,{0x26}},             //VN12    
   {0xD1,1,{0x54}},             //VN8  
   {0xD2,1,{0x66}},             //VN4
   {0xD3,1,{0x3F}},             //VN0 
//qiumeng@wind-mobi.com 20170522 end
 //CMD_P 0
 {0xFF,3,{0x98,0x81,0x00}},
 //qiumeng@wind-mobi.com 20170522 begin
 //{0x51,2,{0x0F,0XFF}},
 //{0x53,1,{0x24}},
 //{0x55,1,{0x00}}, 
 //{0x35,1,{0x00}},   //TE on
 //qiumeng@wind-mobi.com 20170522 end
 {0x11,1,{0x00}},
 {REGFLAG_DELAY, 120, {}},		//120 
 {0x29,1,{0x00}},
 {REGFLAG_DELAY, 20, {}},	   //100
};
/*
static struct LCM_setting_table lcm_sleep_out_setting[] = {
    //Sleep Out
    {0x11, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
    {0x29, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};
*/
static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
    // Display off sequence
    {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 40, {}},

    // Sleep Mode On
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
    {0xFF,	3,		{0x98,0x81,0x01}},
    {0x58, 1, {0x01}},
    {REGFLAG_DELAY, 20, {}},
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
        //modify panel physical size to 5 inches---qiumeng@wind-mobi.com 20170324 begin
		params->physical_width  = 60;
		params->physical_height = 112;
		//modify panel physical size to 5 inches---qiumeng@wind-mobi.com 20170324 end
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
		params->dsi.cont_clock=1;
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
		//disable lcm ssc for A158---qiumeng@wind-mobi.com 20161214 begin		
		params->dsi.ssc_disable=1;
		//disable lcm ssc for A158---qiumeng@wind-mobi.com 20161214 end
		//improve lcm FPS for A158---qiumeng@wind-mobi.com 20170306 begin		
		params->dsi.vertical_sync_active				= 6; //4   
		params->dsi.vertical_backporch				       = 20;  //14  
		params->dsi.vertical_frontporch				       = 14;  //16  
		params->dsi.vertical_active_line				       = FRAME_HEIGHT;     
		params->dsi.horizontal_sync_active				= 40;   //4
		params->dsi.horizontal_backporch				= 40;  //60  
		params->dsi.horizontal_frontporch				= 40;    //60
		params->dsi.horizontal_blanking_pixel				= 60;   
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;  
		//improve lcm FPS for A158---qiumeng@wind-mobi.com 20170306 end
	//	params->dsi.pll_div1=1;		   
	//	params->dsi.pll_div2=1;		   
	//	params->dsi.fbk_div =28;//28	
// To fix rf
		params->dsi.PLL_CLOCK = 212;	   // 245;

		//params->dsi.ss

		//params->dsi.CLK_TRAIL = 17;
			
		params->dsi.esd_check_enable = 1;
		params->dsi.customization_esd_check_enable = 1;
		params->dsi.lcm_esd_check_table[0].cmd                  = 0x0a;
		params->dsi.lcm_esd_check_table[0].count                = 1;
		params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
}

static void lcm_init(void)
{
/***********add for extern avdd ---shenyong@wind-mobi.com---start*********/
	unsigned char cmd = 0x0;
	unsigned char data = 0xFF;
	int ret = 0;
	cmd = 0x00;
	data = 0x0F; //liujinzhou@wind-mobi.com modify at 20171026
/***********add for extern avdd ---shenyong@wind-mobi.com---end*********/
#ifdef GPIO_LCD_BIAS_ENP_PIN	
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);	
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);	
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
#endif
/***********add for extern avdd ---shenyong@wind-mobi.com---start*********/
	MDELAY(5);
	ret = tps65132_write_bytes(cmd, data);
		if (ret < 0)
		printk("ili9881----tps6132----cmd=%0x--i2c write error----\n", cmd);
	else
		printk("ili9881----tps6132----cmd=%0x--i2c write success----\n", cmd);
	cmd = 0x01;
	data = 0x0F; //liujinzhou@wind-mobi.com modify at 20171026
	ret = tps65132_write_bytes(cmd, data);
		if (ret < 0)
		printk("ili9881----tps6132----cmd=%0x--i2c write error----\n", cmd);
	else
		printk("ili9881----tps6132----cmd=%0x--i2c write success----\n", cmd);
	MDELAY(5);
/***********add for extern avdd ---shenyong@wind-mobi.com---end*********/
	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(120);		

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_suspend(void) 
{



	// when phone sleep , config output low, disable backlight drv chip  
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
#ifdef GPIO_LCD_BIAS_ENP_PIN	
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);	
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);	
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);
#endif
	MDELAY(10);

}

static void lcm_resume(void)
{


	lcm_init();

}

//extern unsigned char which_lcd_module_triple_cust(void); 
static unsigned int lcm_compare_id(void)
{
	unsigned int id=0,id1=0,id2=0;
	unsigned char buffer[3];
	unsigned int array[16];
	unsigned char LCD_ID_value = 0;
#ifdef GPIO_LCD_BIAS_ENP_PIN	
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);	
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);	
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
#endif
	MDELAY(10);

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);
	
	SET_RESET_PIN(1);
	MDELAY(120); 


    array[0]=0x00053902;
    array[1]=0x8198FFFF;
    array[2]=0x00000001;
    dsi_set_cmdq(array, 3, 1);
    MDELAY(10); 
    
    array[0] = 0x00023700;// read id return two byte,version and id
    dsi_set_cmdq(array, 1, 1);
    MDELAY(10); 
    
    read_reg_v2(0x00, buffer, 1);
    id1 = buffer[0]; 
    
    read_reg_v2(0x01, buffer, 1);
    id2 = buffer[1];  
    
 //   id2 = buffer[1]; //we test buffer 1
	id = (id1 << 8) | id2;

    #ifdef BUILD_LK
		printf("%s, LK ili9881 debug: ili9881 id = %x,%x,%x\n", __func__, id,id1,id2);
    #else
		printk("%s, kernel ili9881 horse debug: ili9881 id = 0x%08x\n", __func__, id);
    #endif
//return 1;

	if(id == LCM_ID_ILI9881)
	{
		//LCD_ID_value = which_lcd_module_triple_cust();
		printk("%s, LCD_ID_value = %d\n", __func__, LCD_ID_value);
	#ifdef BUILD_LK
		printf("%s, LCD_ID_value = %d\n", __func__, LCD_ID_value);
    #else
		printk("%s, LCD_ID_value = %d\n", __func__, LCD_ID_value);
    #endif
		if(LCD_ID_value == 0x11)
		{
			return 1;
		}
		else
		{
			return 1;
		}
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
    //int i =0;

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

//liujinzhou@wind-mobi.com modify at 20161230 begin
static unsigned int lcm_ata_check(unsigned char *buf)
{
	#ifdef CONFIG_WIND_DEVICE_INFO
	if(!strcmp(g_lcm_name,"ili9881_hd720_dsi_vdo_dj")){
		return 1;
	}else{
		return -1;
	}
	#endif
}
//liujinzhou@wind-mobi.com modify at 20161230 end

LCM_DRIVER ili9881_hd720_dsi_vdo_dj_lcm_drv =
{
    .name           	= "ili9881_hd720_dsi_vdo_dj",
    .set_util_funcs 	= lcm_set_util_funcs,
    .get_params     	= lcm_get_params,
    .init           	= lcm_init,
    .suspend        	= lcm_suspend,
    .resume         	= lcm_resume,
    .compare_id     	= lcm_compare_id,
	.esd_check = lcm_esd_check,
	.esd_recover = lcm_esd_recover,
	.ata_check          = lcm_ata_check,    //sunsiyuan@wind-mobi.com add ata_check at 20161130
};
//late_initcall(lcm_init);
//qiumeng@wind-mobi.com add at 20170425 end

