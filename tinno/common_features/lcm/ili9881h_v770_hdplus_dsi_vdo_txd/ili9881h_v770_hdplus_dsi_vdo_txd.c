#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif

#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <string.h>
#include <platform/mt_i2c.h>
#include <cust_gpio_usage.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#endif



// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define REGFLAG_DELAY                                       0xFCFC
#define REGFLAG_END_OF_TABLE                                0xFDFD  // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE                                    0

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef BUILD_LK
#define LCM_PRINT printf
#else
#if defined(BUILD_UBOOT)
#define LCM_PRINT printf
#else
#define LCM_PRINT printk
#endif
#endif

#define LCM_DBG(fmt, arg...) \
    LCM_PRINT ("[ili9881h_txd] %s (line:%d) :" fmt "\r\n", __func__, __LINE__, ## arg)
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (720)
#define FRAME_HEIGHT (1600)
#define VIRTUAL_WIDTH	(1080)
#define VIRTUAL_HEIGHT	(1920)

/* physical size in um */
#define LCM_PHYSICAL_WIDTH (68000)
#define LCM_PHYSICAL_HEIGHT (152000)
#define LCM_DENSITY	(290)
//static struct LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct  LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))
/*
#define SET_RESET_PIN(v)    \
    mt_set_gpio_mode(GPIO_LCM_RST,GPIO_MODE_00);    \
    mt_set_gpio_dir(GPIO_LCM_RST,GPIO_DIR_OUT);     \
    if(v)                                           \
        mt_set_gpio_out(GPIO_LCM_RST,GPIO_OUT_ONE); \
    else                                            \
        mt_set_gpio_out(GPIO_LCM_RST,GPIO_OUT_ZERO);
*/
#ifdef GPIO_LCM_PWR
#define SET_PWR_PIN(v)    \
    mt_set_gpio_mode(GPIO_LCM_PWR,GPIO_MODE_00);    \
    mt_set_gpio_dir(GPIO_LCM_PWR,GPIO_DIR_OUT);     \
    if(v)                                           \
        mt_set_gpio_out(GPIO_LCM_PWR,GPIO_OUT_ONE); \
    else                                            \
        mt_set_gpio_out(GPIO_LCM_PWR,GPIO_OUT_ZERO);	
#endif
#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

#ifdef BUILD_LK
#define GPIO_LCD_ID (GPIO178 | 0x80000000)         
#else
extern void lcm_enn(int onoff);
extern void lcm_enp(int onoff);
extern  int _lcm_i2c_write_bytes(unsigned char addr, unsigned char value);
extern int get_lcm_id_status(void);
#endif

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V3(para_tbl,size,force_update)        lcm_util.dsi_set_cmdq_V3(para_tbl,size,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

extern void lcm_enn(int onoff);
extern void lcm_enp(int onoff);
extern  int _lcm_i2c_write_bytes(unsigned char addr, unsigned char value);
extern int get_lcm_id_status(void);

struct LCM_setting_table
{
    unsigned int cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table  lcm_deep_sleep_mode_in_setting_v2[] = {
    // Display off sequence
    {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 10, {}},

    // Sleep Mode On
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 60, {}},
};

/***
static struct LCM_setting_table lcm_initialization_setting_v2[] = 
{
{0xFF,3,{0x98,0x81,0x03}},
{0x01,1,{0x00}},
{0x02,1,{0x00}},
{0x03,1,{0x73}},
{0x04,1,{0x73}},
{0x05,1,{0x00}},
{0x06,1,{0x06}},
{0x07,1,{0x02}},
{0x08,1,{0x00}},
{0x09,1,{0x01}},
{0x0a,1,{0x01}},
{0x0b,1,{0x01}},
{0x0c,1,{0x01}},
{0x0d,1,{0x01}},
{0x0e,1,{0x01}},
{0x0f,1,{0x00}},   //32：2.5T  00:3T
{0x10,1,{0x00}},   //32：2.5T  00:3T
{0x11,1,{0x00}},
{0x12,1,{0x00}},
{0x13,1,{0x01}},
{0x14,1,{0x00}},
{0x15,1,{0x00}},
{0x16,1,{0x00}},
{0x17,1,{0x00}},
{0x18,1,{0x00}},
{0x19,1,{0x00}},
{0x1a,1,{0x00}},
{0x1b,1,{0x00}},
{0x1c,1,{0x00}},
{0x1d,1,{0x00}},
{0x1e,1,{0xC0}},
{0x1f,1,{0x80}},
{0x20,1,{0x03}},
{0x21,1,{0x04}},
{0x22,1,{0x00}},
{0x23,1,{0x00}},
{0x24,1,{0x00}},
{0x25,1,{0x00}},
{0x26,1,{0x00}},
{0x27,1,{0x00}},
{0x28,1,{0x33}},
{0x29,1,{0x02}},
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
{0x3a,1,{0x00}},
{0x3b,1,{0x00}},
{0x3c,1,{0x00}},
{0x3d,1,{0x00}},
{0x3e,1,{0x00}},
{0x3f,1,{0x00}},
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
{0x5e,1,{0x10}},
{0x5f,1,{0x09}},
{0x60,1,{0x08}},
{0x61,1,{0x0F}},
{0x62,1,{0x0E}},
{0x63,1,{0x0D}},
{0x64,1,{0x0C}},
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
{0x71,1,{0x06}},
{0x72,1,{0x07}},
{0x73,1,{0x02}},
{0x74,1,{0x02}},
{0x75,1,{0x06}},
{0x76,1,{0x07}},
{0x77,1,{0x0E}},
{0x78,1,{0x0F}},
{0x79,1,{0x0C}},
{0x7a,1,{0x0D}},
{0x7b,1,{0x02}},
{0x7c,1,{0x02}},
{0x7D,1,{0x02}},
{0x7E,1,{0x02}},
{0x7F,1,{0x02}},
{0x80,1,{0x02}},
{0x81,1,{0x02}},
{0x82,1,{0x02}},
{0x83,1,{0x02}},
{0x84,1,{0x02}},
{0x85,1,{0x02}},
{0x86,1,{0x02}},
{0x87,1,{0x09}},
{0x88,1,{0x08}},
{0x89,1,{0x02}},
{0x8A,1,{0x02}},
{0xFF,3,{0x98,0x81,0x04}},
{0x6C,1,{0x15}},
{0x6E,1,{0x2A}},     //15V
{0x6F,1,{0x33}},
{0x3A,1,{0x24}},
{0x8D,1,{0x1A}},   //-11V
{0x87,1,{0xBA}},
{0x26,1,{0x76}},
{0xB2,1,{0xD1}},
{0xB5,1,{0x07}},
{0x35,1,{0x1F}},
{0x33,1,{0x14}},
{0x3B,1,{0x98}},
{0x38,1,{0x01}},
{0x39,1,{0x00}},
{0xFF,3,{0x98,0x81,0x01}},
{0x22,1,{0x0A}},
{0x31,1,{0x00}},
{0x53,1,{0x40}},
{0x55,1,{0x40}},
{0x50,1,{0xAE}},    //4.788
{0x51,1,{0xA9}},   //-4.778
{0x60,1,{0x16}},
{0x62,1,{0x20}},
{0x40,1,{0x33}},
{0x2E,1,{0xF0}},    //1440 GATE NL SEL
{0xA0,1,{0x00}},
{0xA1,1,{0x23}},
{0xA2,1,{0x37}},
{0xA3,1,{0x1A}},
{0xA4,1,{0x1F}},
{0xA5,1,{0x33}},
{0xA6,1,{0x27}},
{0xA7,1,{0x25}},
{0xA8,1,{0xAE}},
{0xA9,1,{0x1C}},
{0xAA,1,{0x27}},
{0xAB,1,{0x8D}},
{0xAC,1,{0x1C}},
{0xAD,1,{0x1C}},
{0xAE,1,{0x52}},
{0xAF,1,{0x26}},
{0xB0,1,{0x2B}},
{0xB1,1,{0x55}},
{0xB2,1,{0x5F}},
{0xB3,1,{0x23}},
{0xC0,1,{0x00}},
{0xC1,1,{0x23}},
{0xC2,1,{0x37}},
{0xC3,1,{0x1A}},
{0xC4,1,{0x1F}},
{0xC5,1,{0x33}},
{0xC6,1,{0x27}},
{0xC7,1,{0x26}},
{0xC8,1,{0xAE}},
{0xC9,1,{0x1C}},
{0xCA,1,{0x27}},
{0xCB,1,{0x8D}},
{0xCC,1,{0x1C}},
{0xCD,1,{0x1C}},
{0xCE,1,{0x51}},
{0xCF,1,{0x26}},
{0xD0,1,{0x2C}},
{0xD1,1,{0x55}},
{0xD2,1,{0x5F}},
{0xD3,1,{0x23}},
{0xFF,3,{0x98,0x81,0x00}},
{0x35,1,{0x00}},
{0x11,1,{0x00}},
{REGFLAG_DELAY, 100,{}},
{0x29,1,{0x00}},
{REGFLAG_DELAY, 20,{}},
{REGFLAG_END_OF_TABLE, 0x00,{}}

};
***/

static struct LCM_setting_table lcm_initialization_setting[] = 
{
{0x11,1,{0x00}},
{REGFLAG_DELAY, 100,{}},
{0x29,1,{0x00}},
{REGFLAG_DELAY, 20,{}},
{REGFLAG_END_OF_TABLE, 0x00,{}}

};



// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;
    for(i = 0; i < count; i++)
    {
        unsigned cmd;
        cmd = table[i].cmd;
        switch (cmd)
        {
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


static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}


static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));
	
	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	params->physical_width = LCM_PHYSICAL_WIDTH/1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT/1000;

	params->density		   = LCM_DENSITY;

    params->dbi.te_mode                 = LCM_DBI_TE_MODE_DISABLED;

    #if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
    #else
	params->dsi.mode   = BURST_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; 
    #endif
	
		// DSI
		/* Command mode setting */
		//1 Three lane or Four lane
	params->dsi.LANE_NUM				= LCM_THREE_LANE;
		//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.packet_size=256;
    // Video mode setting
    params->dsi.intermediat_buffer_num = 2;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		
    params->dsi.vertical_sync_active                = 2;
    params->dsi.vertical_backporch              = 16;
    params->dsi.vertical_frontporch             = 249;
	params->dsi.vertical_active_line = FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active              = 25;
    params->dsi.horizontal_backporch                = 26;
    params->dsi.horizontal_frontporch               = 25; 
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
    params->dsi.compatibility_for_nvk = 0;
    params->dsi.ssc_disable=1;
    params->dsi.ssc_range=2;
	params->dsi.PLL_CLOCK=382;
		
#ifndef BUILD_LK	
	params->dsi.esd_check_enable = 1; 
	params->dsi.customization_esd_check_enable = 0;//0:te esd check 1:read register
	//params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	//params->dsi.lcm_esd_check_table[0].count = 1;
	//params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;

	//params->dsi.lcm_esd_check_table[1].cmd = 0x0D;
	//params->dsi.lcm_esd_check_table[1].count = 1;
//	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x00;
#endif /*BUILD_LK*/

#ifdef CONFIG_MTK_ROUND_CORNER_SUPPORT
	params->round_corner_en = 1;
	params->full_content = 0;
	params->corner_pattern_width = 32;
	params->corner_pattern_height = 32;
	params->corner_pattern_height_bot = 20;
#endif
}

#ifdef BUILD_LK
static int gpio_bl_enp   = (150 | 0x80000000);      
static int gpio_bl_enn   = (151 | 0x80000000);   
static struct mt_i2c_t LCD_i2c;

static kal_uint32 lcd_write_byte(kal_uint8 addr, kal_uint8 value)
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;

    write_data[0]= addr;
    write_data[1] = value;

    LCD_i2c.id = I2C6;

    LCD_i2c.addr = (0x11);                 //KTZ8864
    LCD_i2c.mode = ST_MODE;
    LCD_i2c.speed = 100;
    len = 2;

    ret_code = i2c_write(&LCD_i2c, write_data, len);

    return ret_code;
}
#endif

static void lcm_init(void)
{
    LCM_DBG();
#ifdef BUILD_LK
    lcd_write_byte(0x0D,0x1E);
    MDELAY(1);
    lcd_write_byte(0x0E,0x1E);
    MDELAY(1);
    lcd_write_byte(0x09,0x99);
    MDELAY(1);
    mt_set_gpio_mode(gpio_bl_enp,GPIO_MODE_00);
    mt_set_gpio_dir(gpio_bl_enp,GPIO_DIR_OUT);
    mt_set_gpio_mode(gpio_bl_enn,GPIO_MODE_00);
    mt_set_gpio_dir(gpio_bl_enn,GPIO_DIR_OUT);
    mt_set_gpio_out(gpio_bl_enp,GPIO_OUT_ONE);
    mt_set_gpio_out(gpio_bl_enn,GPIO_OUT_ONE);
    MDELAY(10);
    lcd_write_byte(0x02,0x69);
    MDELAY(1);
    lcd_write_byte(0x08,0x1F);
#else //Kernel driver
    _lcm_i2c_write_bytes(0x0D,0x1E);
    MDELAY(1);
    _lcm_i2c_write_bytes(0x0E,0x1E);
    MDELAY(1);
    _lcm_i2c_write_bytes(0x09,0x99);
    MDELAY(1);
    lcm_enp(1);
    lcm_enn(1);
    MDELAY(10);
    _lcm_i2c_write_bytes(0x02,0x69);
    MDELAY(1);
    _lcm_i2c_write_bytes(0x08,0x1F);
#endif
#ifdef GPIO_LCM_PWR
    SET_PWR_PIN(0);
    MDELAY(20);
    SET_PWR_PIN(1);
    MDELAY(150);
#endif
    SET_RESET_PIN(1);
    MDELAY(5);
    SET_RESET_PIN(0);
    MDELAY(20);
    SET_RESET_PIN(1);
    MDELAY(110);
    LCM_DBG("debug,lcm reset end \n");
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
    LCM_DBG("debug,lcm init end \n");
}

static void lcm_suspend(void)
{
    LCM_DBG();
#ifndef BUILD_LK
    //LCM_DBG("geroge suspend bEnTGesture  = %d\n",bEnTGesture);

    push_table(lcm_deep_sleep_mode_in_setting_v2, sizeof(lcm_deep_sleep_mode_in_setting_v2) / sizeof(struct LCM_setting_table), 1);

        //SET_RESET_PIN(1);
       // SET_RESET_PIN(0);
       // MDELAY(10); // 1ms


    lcm_enp(0);
    MDELAY(1);
    lcm_enn(0);
    MDELAY(1);

      //  SET_RESET_PIN(1);
     //   MDELAY(50);


#endif

}

static void lcm_resume(void)
{

#ifndef BUILD_LK
    //LCM_DBG("geroge resume bEnTGesture  = %d\n",bEnTGesture);
    lcm_enp(1);
    MDELAY(1);
    lcm_enn(1);
#endif
    MDELAY(1);
    SET_RESET_PIN(0);
    MDELAY(5);
    SET_RESET_PIN(1);
    MDELAY(50);
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
    LCM_DBG();
}
  

static unsigned int lcm_compare_id(void)
{
    s32 lcd_hw_id = -1;

#ifdef BUILD_LK
    mt_set_gpio_mode(GPIO_LCD_ID,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCD_ID,GPIO_DIR_IN);
    lcd_hw_id = mt_get_gpio_in(GPIO_LCD_ID);
#else
  //  lcd_hw_id = get_lcm_id_status();
#endif
    LCM_DBG("lcm_compare_id lcd_hw_id=%d \n",lcd_hw_id);
   // if (1 == lcd_hw_id)
  //  {
  //      return 1;
//    }
 //   else
   // {
   //     return 0;
   // }
	return 0;
}

struct LCM_DRIVER ili9881h_v770_hdplus_dsi_vdo_txd_lcm_drv = 
{
	.name		= "ili9881h_hdplus_dsi_vdo_txd",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
};
