//xuzhifeng@wind-mobi.com add at 20161108 begin

#include "lcm_drv.h"

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


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define LCM_DSI_CMD_MODE									0
#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)
#define LCM_ID_ILI9881                                                              (0x9800)    

//sunsiyuan@wind-mobi.com modify ata_check at 20161231 begin
#ifdef CONFIG_WIND_DEVICE_INFO
		extern char *g_lcm_name;
#endif
//sunsiyuan@wind-mobi.com modify ata_check at 20161231 end

#define REGFLAG_DELAY             								0xFC
#define REGFLAG_END_OF_TABLE      							0xFD   // END OF REGISTERS MARKER

#define GPIO_LCD_BIAS_ENP_PIN         (GPIO78 | 0x80000000)

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

//tuwenzan@wind-mobi.com add for init cabc ctrl backlight at 20170106 begin
#ifdef CONFIG_WIND_CABC_MODE_SUPPORT
extern int cabc_mode_mode;
#define CABC_MODE_SETTING_UI	1
#define CABC_MODE_SETTING_MV	2
#define CABC_MODE_SETTING_DIS	3
#define CABC_MODE_SETTING_NULL	0
#endif
//tuwenzan@wind-mobi.com add for init cabc ctrl backlight at 20170106 end

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
	//liujinzhou@wind-mobi.com modify at 20161223 begin
	{0xFF,3,{0x98,0x81,0x03}},
	//GIP_1                   
	{0x01,1,{0x00}},
	{0x02,1,{0x00}},
	{0x03,1,{0x73}},
	{0x04,1,{0xD3}},
	{0x05,1,{0x00}},
	{0x06,1,{0x0A}},
	{0x07,1,{0x0E}},
	{0x08,1,{0x00}},
	{0x09,1,{0x01}},
	{0x0a,1,{0x01}},
	{0x0b,1,{0x01}},
	{0x0c,1,{0x01}},
	{0x0d,1,{0x01}},
	{0x0e,1,{0x01}},
	{0x0f,1,{0x15}},
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
	{0x1a,1,{0x00}},
	{0x1b,1,{0x00}},
	{0x1c,1,{0x00}},
	{0x1d,1,{0x00}},
	{0x1e,1,{0x40}},
	{0x1f,1,{0x80}},  
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
	{0x2a,1,{0x00}},
	{0x2b,1,{0x00}},
	{0x2c,1,{0x00}},
	{0x2d,1,{0x00}},
	{0x2e,1,{0x00}},
	{0x2f,1,{0x00}},
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
	{0x3a,1,{0x40}},
	{0x3b,1,{0x40}},
	{0x3c,1,{0x00}},
	{0x3d,1,{0x00}},
	{0x3e,1,{0x00}},
	{0x3f,1,{0x00}},
	{0x40,1,{0x00}},
	{0x41,1,{0x00}},
	{0x42,1,{0x00}},
	{0x43,1,{0x00}},
	{0x44,1,{0x00}},
	
		//GIP_2           
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
	
	//GIP_3        
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
	{0x8A,1,{0x01}},
	
		//CMD_Page 4       
	{0xFF,3,{0x98,0x81,0x04}},
	{0x6C,1,{0x15}},
	{0x6E,1,{0x3B}},               //di_pwr_reg=0 VGH clamp 
	{0x6F,1,{0x37}},              // reg vcl + VGH pumping ratio 3x VGL=-2x            
	{0x8D,1,{0x14}},               //VGL clamp -12V
	{0x3A,1,{0x24}},   
	{0x35,1,{0x1F}},     
	{0x87,1,{0xBA}},		//5.2 none
	{0x26,1,{0x76}},          //5.2 none
	{0xb2,1,{0xd1}},
	{0xb5,1,{0x06}},
	{0x31,1,{0x75}},
	
		//CMD_Page 1        
	{0xFF,3,{0x98,0x81,0x01}},
	{0x22,1,{0x0A}},		//BGR, SS
	{0x31,1,{0x00}},		//column inversion
	{0x50,1,{0x8D}},         	//VREG1OUT=4.104V
	{0x51,1,{0x98}},         	//VREG2OUT=-4.106V
	{0x60,1,{0x1F}},               //SDT

	{0xA0,1,{0x1D}},		//VP255	Gamma P
	{0xA1,1,{0x30}},		//VP251	     
	{0xA2,1,{0x3C}},		//VP247  
	{0xA3,1,{0x13}},		//VP243 
	{0xA4,1,{0x15}},		//VP239  
	{0xA5,1,{0x26}},		//VP231  
	{0xA6,1,{0x1B}},		//VP219  
	{0xA7,1,{0x1E}},		//VP203  
	{0xA8,1,{0xA1}},		//VP175 
	{0xA9,1,{0x1B}},		//VP144 
	{0xAA,1,{0x29}},		//VP111   
	{0xAB,1,{0x8A}},		//VP80  
	{0xAC,1,{0x1E}},		//VP52 
	{0xAD,1,{0x1F}},		//VP36 
	{0xAE,1,{0x49}},		//VP24
	{0xAF,1,{0x53}},		//VP16  
	{0xB0,1,{0x2C}},		//VP12  
	{0xB1,1,{0x4C}},		//VP8  
	{0xB2,1,{0x56}},		//VP4   
	{0xB3,1,{0x2B}},		//VP0                        
	
	{0xC0,1,{0x1D}},		//VN255 GAMMA N  		
	{0xC1,1,{0x30}},		//VN251
	{0xC2,1,{0x3C}},		//VN247    
	{0xC3,1,{0x13}},		//VN243  
	{0xC4,1,{0x15}},		//VN239   
	{0xC5,1,{0x28}},		//VN231   
	{0xC6,1,{0x1B}},		//VN219    
	{0xC7,1,{0x1E}},		//VN203  
	{0xC8,1,{0xA1}},		//VN175   
	{0xC9,1,{0x1B}},		//VN144   
	{0xCA,1,{0x29}},		//VN111  
	{0xCB,1,{0x8A}},		//VN80    
	{0xCC,1,{0x1E}},		//VN52   
	{0xCD,1,{0x1F}},		//VN36  
	{0xCE,1,{0x53}},		//VN24 
	{0xCF,1,{0x27}},		//VN16  
	{0xD0,1,{0x2C}},		//VN12      
	{0xD1,1,{0x4C}},		//VN8 
	{0xD2,1,{0x56}},		//VN4    
	{0xD3,1,{0x2B}},		//VN0                
	
		//CMD_Page 0        
	{0xFF,3,{0x98,0x81,0x00}},
//tuwenzan@wind-mobi.com add for init cabc ctrl backlight at 20170105 begin
	{0x35,1,{0x00}},
	{0x11,1,{0x00}},
	{REGFLAG_DELAY, 120, {}},		//120 
	{0x29,1,{0x00}},
	{REGFLAG_DELAY, 20, {}},	   //20

#ifdef  CONFIG_WIND_CABC_BACKLIGHT_CTRL
	{0x51,2,{0x00,0x00}}, 
	{0x53,1,{0x2C}},
	{0x55,1,{0x00}},
#endif
//tuwenzan@wind-mobi.com add for init cabc ctrl backlight at 20170105 end
	{REGFLAG_END_OF_TABLE, 0x00, {}}
	//liujinzhou@wind-mobi.com modify at 20161223 end
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
//tuwenzan@wind-mobi.com add for init cabc ctrl backlight at 20170105 begin
#ifdef CONFIG_WIND_CABC_MODE_SUPPORT
static struct LCM_setting_table lcm_setting_ui[] = {
{0x55,1,{0x01}},
{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_setting_dis[] = {
{0x55,1,{0x00}},
{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_setting_mv[] = {
{0x55,1,{0x03}},
{REGFLAG_END_OF_TABLE, 0x00, {}}
};
#endif

#ifdef  CONFIG_WIND_CABC_BACKLIGHT_CTRL
extern int wind_board_id;
static struct LCM_setting_table lcm_backlight_level_setting[] = {
	{ 0x51, 2, {0x0f,0xFF} },
	{ REGFLAG_END_OF_TABLE, 0x00, {} }
};
static struct LCM_setting_table close_lcm_backlight_setting[] = {
	{0x51,2,{0x00,0x00}},
	{0x53,1,{0x00}},
	{0x55,1,{0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
#endif
//tuwenzan@wind-mobi.com add for init cabc ctrl backlight at 20170105 end

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
		params->dsi.vertical_sync_active				= 6; //4   
		params->dsi.vertical_backporch				       = 20;  //14  
		params->dsi.vertical_frontporch				       = 14;  //16  
		params->dsi.vertical_active_line				       = FRAME_HEIGHT; 
		//tuwenzan@wind-mobi.com modify for frame rate at 20170210 begin
		params->dsi.horizontal_sync_active				= 40;   //4
		params->dsi.horizontal_backporch				= 40;  //60  
		params->dsi.horizontal_frontporch				= 40;    //60
		//tuwenzan@wind-mobi.com modify for frame rate at 20170210 end
		params->dsi.horizontal_blanking_pixel				= 60;   
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;  
		
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
		//tuwenzan@wind-mobi.com add physical size at 20170217 begin
		params->physical_width  = 118;
		params->physical_height = 65;
		//tuwenzan@wind-mobi.com add physical size at 20170217 begin
}

static void lcm_init(void)
{
#ifdef GPIO_LCD_BIAS_ENP_PIN	
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);	
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);	
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
#endif
	MDELAY(10);

	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(120);		

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1); 
	//tuwenzan@wind-mobi.com add for init cabc ctrl backlight at 20170105 begin
	#ifdef  CONFIG_WIND_CABC_BACKLIGHT_CTRL
	if(wind_board_id <= 0x13){
		push_table(close_lcm_backlight_setting, sizeof(close_lcm_backlight_setting) / sizeof(struct LCM_setting_table), 1);
	}
	#endif
	//tuwenzan@wind-mobi.com add for init cabc ctrl backlight at 20170105 end
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
printk("hyg ---- lcm_resume enter \n");
//huyunge@wind-mobi.com modify at 20170322 for LCD out sleep time 
#ifdef GPIO_LCD_BIAS_ENP_PIN	
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);	
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);	
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
#endif
	MDELAY(10);

	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(10);		

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1); 
	#ifdef  CONFIG_WIND_CABC_BACKLIGHT_CTRL
	if(wind_board_id <= 0x13){
		push_table(close_lcm_backlight_setting, sizeof(close_lcm_backlight_setting) / sizeof(struct LCM_setting_table), 1);
	}
	#endif
	//lcm_init();
	printk("hyg ---- lcm_resume out \n");
//huyunge@wind-mobi.com modify at 20170322 for LCD out sleep time
}

//sunsiyuan@wind-mobi.com modify ata_check at 20161228 begin
//tuwenzan@wind-mobi.com add for init cabc ctrl backlight at 20170105 begin
#ifdef CONFIG_WIND_CABC_MODE_SUPPORT
static void lcm_cabc_cmdq(void *handle, unsigned int mode)
{
	printk("tuwenzan enter lcm_cabc_cmdq mode = %d\n",mode);
	if(wind_board_id > 0x13){
	switch(mode)
	{
		case CABC_MODE_SETTING_UI:
			{
				push_table(lcm_setting_ui,
			   sizeof(lcm_setting_ui) / sizeof(struct LCM_setting_table), 1);
			}
		break;
		case CABC_MODE_SETTING_MV:
			{
				push_table(lcm_setting_mv,
			   sizeof(lcm_setting_mv) / sizeof(struct LCM_setting_table), 1);
			}
		break;
		case CABC_MODE_SETTING_DIS:
			{
				push_table(lcm_setting_dis,
			   sizeof(lcm_setting_dis) / sizeof(struct LCM_setting_table), 1);
			}
		break;
		default:
		{
			push_table(lcm_setting_dis,
			   sizeof(lcm_setting_dis) / sizeof(struct LCM_setting_table), 1);
		}

	}
		}
}
#endif


#ifdef  CONFIG_WIND_CABC_BACKLIGHT_CTRL
static void lcm_setbacklight(unsigned int level)
{
	unsigned int level_hight,level_low=0;
	unsigned int mapped_level = 0;
	if(wind_board_id > 0x13){
    #ifdef WIND_LCM_MAPP_BACKLIGHT
	if (level > 102)
		mapped_level = (641*level + 36667)/(1000);
	#else
		mapped_level=level;
	#endif
	MDELAY(5);	
	level_hight=(mapped_level & 0xf0)>>4;
	level_low=(mapped_level & 0x0f)<<4;
	lcm_backlight_level_setting[0].para_list[0] = level_hight;
	lcm_backlight_level_setting[0].para_list[1] = level_low;
	push_table(lcm_backlight_level_setting,
		   sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
	}
}
#endif
//tuwenzan@wind-mobi.com add for init cabc ctrl backlight at 20170105 end
static unsigned int lcm_ata_check(unsigned char *buf)
{
	#ifdef CONFIG_WIND_DEVICE_INFO
	if(!strcmp(g_lcm_name,"ili9881_hd720_dsi_vdo_tm")){
		return 1;
	}else{
		return -1;
	}
	#endif
}
//sunsiyuan@wind-mobi.com modify ata_check at 20161228 end
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
 	//liujinzhou@wind-mobi.com modify at 20161130 begin
    else
    {
        return 0;//1->0
    }
	//liujinzhou@wind-mobi.com modify at 20161130 end
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
LCM_DRIVER ili9881_hd720_dsi_vdo_tm_lcm_drv =
{
    .name           	= "ili9881_hd720_dsi_vdo_tm",
    .set_util_funcs 	= lcm_set_util_funcs,
    .get_params     	= lcm_get_params,
    .init           	= lcm_init,
    .suspend        	= lcm_suspend,
    .resume         	= lcm_resume,
    .compare_id     	= lcm_compare_id,
	.esd_check = lcm_esd_check,
	.esd_recover = lcm_esd_recover,
//tuwenzan@wind-mobi.com add for init cabc ctrl backlight at 20170105 begin
#ifdef  CONFIG_WIND_CABC_BACKLIGHT_CTRL
	.set_backlight = lcm_setbacklight,
#endif
#ifdef CONFIG_WIND_CABC_MODE_SUPPORT
	.set_cabc_cmdq = lcm_cabc_cmdq,
#endif

//tuwenzan@wind-mobi.com add for init cabc ctrl backlight at 20170105 end
	.ata_check          = lcm_ata_check,    //sunsiyuan@wind-mobi.com add ata_check at 20161128
};
//late_initcall(lcm_init);
//xuzhifeng@wind-mobi.com add at 20161108 end

