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
    LCM_PRINT ("[vtdr6110c_tm] %s (line:%d) :" fmt "\r\n", __func__, __LINE__, ## arg)
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (1080)
#define FRAME_HEIGHT (2400)
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

static struct LCM_setting_table lcm_initialization_setting[] = 
{
{0x35,1,{0x00}},
{0x53,1,{0x20}},
{0x51,2,{0x07,0xff}},
{0x6f,1,{0x01}},
{0x59,1,{0x09}},
{0xF0,2,{0xAA,0x10}},
{0xCF,4,{0x0D,0x87,0x0B,0x12}},
{0xD0,17,{0x84,0x15,0x50,0x14,0x14,0x00,0x29,0x2C,0x14,0x19,0x32,0x00,0x2C,0x14,0x19,0x32,0x00}},
{0xEA,2,{0x30,0x00}},
{0xF0,2,{0xAA,0x11}},
{0xBB,2,{0x10,0x21}},
{0xF0,2,{0xAA,0x12}},
{0xC7,4,{0x8F,0x54,0xA5,0x02}},
{0xF0,2,{0xAA,0x15}},
{0xBE,11,{0x22,0x11,0x22,0x22,0x11,0x11,0x00,0x00,0x00,0x00,0x00}},
{0xF0,2,{0xAA,0x00}},
{0x11,1,{0x00}},
{REGFLAG_DELAY, 120,{}},
{0x29,1,{0x00}},
{REGFLAG_DELAY, 100,{}}


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
	params->dsi.LANE_NUM				= LCM_FOUR_LANE;
		//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.packet_size=256;
    // Video mode setting
    params->dsi.intermediat_buffer_num = 2;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		
    params->dsi.vertical_sync_active                = 4;
    params->dsi.vertical_backporch              = 14;
    params->dsi.vertical_frontporch             = 16;
	params->dsi.vertical_active_line = FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active              = 4;
    params->dsi.horizontal_backporch                = 40;
    params->dsi.horizontal_frontporch               = 40; 
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
    params->dsi.compatibility_for_nvk = 0;
    params->dsi.ssc_disable=1;
    params->dsi.ssc_range=2;
	params->dsi.PLL_CLOCK=170;
		
#ifndef BUILD_LK	
	params->dsi.esd_check_enable = 0; 
	params->dsi.customization_esd_check_enable = 0;//0:te esd check 1:read register
	//params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	//params->dsi.lcm_esd_check_table[0].count = 1;
	//params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;

	//params->dsi.lcm_esd_check_table[1].cmd = 0x0D;
	//params->dsi.lcm_esd_check_table[1].count = 1;
//	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x00;
#endif /*BUILD_LK*/
}

#ifdef BUILD_LK
static int gpio_bl_enp   = (150 | 0x80000000);      
static int gpio_bl_enn   = (175 | 0x80000000);   
static struct mt_i2c_t LCD_i2c;

static kal_uint32 lcd_write_byte(kal_uint8 addr, kal_uint8 value)
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;

    write_data[0]= addr;
    write_data[1] = value;

    LCD_i2c.id = I2C0;

    LCD_i2c.addr = (0x3E);                 //OCP2131
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
    mt_set_gpio_mode(gpio_bl_enp,GPIO_MODE_00);
    mt_set_gpio_dir(gpio_bl_enp,GPIO_DIR_OUT);
    mt_set_gpio_mode(gpio_bl_enn,GPIO_MODE_00);
    mt_set_gpio_dir(gpio_bl_enn,GPIO_DIR_OUT);
    mt_set_gpio_out(gpio_bl_enp,GPIO_OUT_ONE);
    mt_set_gpio_out(gpio_bl_enn,GPIO_OUT_ONE);
    MDELAY(10);
#else //Kernel driver
    lcm_enp(1);
    lcm_enn(1);
    MDELAY(10);
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
    push_table(lcm_deep_sleep_mode_in_setting_v2, sizeof(lcm_deep_sleep_mode_in_setting_v2) / sizeof(struct LCM_setting_table), 1);
    lcm_enp(0);
    MDELAY(1);
    lcm_enn(0);
    MDELAY(1);
#endif
}

static void lcm_resume(void)
{

#ifndef BUILD_LK
    lcm_enp(1);
    MDELAY(1);
    lcm_enn(1);
    MDELAY(1);
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

struct LCM_DRIVER vtdr6110c_p510_hdplus_dsi_vdo_tm_lcm_drv = 
{
	.name		= "vtdr6110c_p510_hdplus_dsi_vdo_tm",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
};
