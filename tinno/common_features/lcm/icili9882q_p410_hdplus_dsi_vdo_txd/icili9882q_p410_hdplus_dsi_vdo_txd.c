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
#define REGFLAG_UDELAY			0xFFFB
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
    LCM_PRINT ("[icili9882q_tm] %s (line:%d) :" fmt "\r\n", __func__, __LINE__, ## arg)
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
#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)


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
	{REGFLAG_DELAY, 1, {}},
    {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},

    // Sleep Mode On
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 80, {}},
};

static struct LCM_setting_table lcm_initialization_setting[] = 
{
{0xFF,3,{0x98,0x82,0x01}},
{0x00,1,{0x47}},
{0x01,1,{0x38}},
{0x02,1,{0x00}},
{0x03,1,{0x00}},
{0x04,1,{0x43}},
{0x05,1,{0x13}},
{0x06,1,{0x00}},
{0x07,1,{0x00}},
{0x08,1,{0x82}},
{0x09,1,{0x83}},
{0x0A,1,{0xF3}},
{0x0B,1,{0x00}},
{0x16,1,{0x82}},
{0x17,1,{0x83}},
{0x18,1,{0x73}},
{0x19,1,{0x00}},
{0x0C,1,{0x00}},
{0x0D,1,{0x00}},
{0x0E,1,{0x00}},
{0x0F,1,{0x00}},
{0x1A,1,{0x00}},
{0x1B,1,{0x00}},
{0x1C,1,{0x00}},
{0x1D,1,{0x00}},
{0x24,1,{0x01}},
{0x25,1,{0x0B}},
{0x31,1,{0x07}},
{0x32,1,{0x0D}},
{0x33,1,{0x21}},
{0x34,1,{0x00}},
{0x35,1,{0x01}},
{0x36,1,{0x02}},
{0x37,1,{0x1F}},
{0x38,1,{0x1D}},
{0x39,1,{0x1B}},
{0x3A,1,{0x19}},
{0x3B,1,{0x17}},
{0x3C,1,{0x15}},
{0x3D,1,{0x13}},
{0x3E,1,{0x11}},
{0x3F,1,{0x0B}},
{0x40,1,{0x09}},
{0x41,1,{0x07}},
{0x42,1,{0x02}},
{0x43,1,{0x07}},
{0x44,1,{0x07}},
{0x45,1,{0x07}},
{0x46,1,{0x07}},
{0x47,1,{0x07}},
{0x48,1,{0x0C}},
{0x49,1,{0x20}},
{0x4A,1,{0x00}},
{0x4B,1,{0x01}},
{0x4C,1,{0x02}},
{0x4D,1,{0x1E}},
{0x4E,1,{0x1C}},
{0x4F,1,{0x1A}},
{0x50,1,{0x18}},
{0x51,1,{0x16}},
{0x52,1,{0x14}},
{0x53,1,{0x12}},
{0x54,1,{0x10}},
{0x55,1,{0x0A}},
{0x56,1,{0x08}},
{0x57,1,{0x07}},
{0x58,1,{0x02}},
{0x59,1,{0x07}},
{0x5A,1,{0x07}},
{0x5B,1,{0x07}},
{0x5C,1,{0x07}},
{0x61,1,{0x07}},
{0x62,1,{0x0A}},
{0x63,1,{0x08}},
{0x64,1,{0x00}},
{0x65,1,{0x01}},
{0x66,1,{0x02}},
{0x67,1,{0x10}},
{0x68,1,{0x12}},
{0x69,1,{0x14}},
{0x6A,1,{0x16}},
{0x6B,1,{0x18}},
{0x6C,1,{0x1A}},
{0x6D,1,{0x1C}},
{0x6E,1,{0x1E}},
{0x6F,1,{0x0C}},
{0x70,1,{0x20}},
{0x71,1,{0x07}},
{0x72,1,{0x02}},
{0x73,1,{0x07}},
{0x74,1,{0x07}},
{0x75,1,{0x07}},
{0x76,1,{0x07}},
{0x77,1,{0x07}},
{0x78,1,{0x0B}},
{0x79,1,{0x09}},
{0x7A,1,{0x00}},
{0x7B,1,{0x01}},
{0x7C,1,{0x02}},
{0x7D,1,{0x11}},
{0x7E,1,{0x13}},
{0x7F,1,{0x15}},
{0x80,1,{0x17}},
{0x81,1,{0x19}},
{0x82,1,{0x1B}},
{0x83,1,{0x1D}},
{0x84,1,{0x1F}},
{0x85,1,{0x0D}},
{0x86,1,{0x21}},
{0x87,1,{0x07}},
{0x88,1,{0x02}},
{0x89,1,{0x07}},
{0x8A,1,{0x07}},
{0x8B,1,{0x07}},
{0x8C,1,{0x07}},
{0xD1,1,{0x00}},
{0xE2,1,{0x00}},
{0xE6,1,{0x22}},
{0xE7,1,{0x54}},
{0xFF,3,{0x98,0x82,0x02}},
{0xF1,1,{0x1C}},
{0x4B,1,{0x5A}},
{0x50,1,{0xCA}},
{0x51,1,{0x00}},
{0x06,1,{0x58}},
{0x0B,1,{0xA0}},
{0x0C,1,{0x80}},
{0x0D,1,{0x1A}},
{0x0E,1,{0x6E}},
{0x4E,1,{0x11}},
{0x4D,1,{0xCE}},
{0xF2,1,{0x4A}},
{0xFF,3,{0x98,0x82,0x05}},
{0x03,1,{0x00}},
{0x04,1,{0xFF}},
{0x63,1,{0x7E}},
{0x64,1,{0x7E}},
{0x68,1,{0xA1}},
{0x69,1,{0xA7}},
{0x6A,1,{0xA1}},
{0x6B,1,{0x93}},
{0x85,1,{0x37}},
{0xFF,3,{0x98,0x82,0x06}},
{0xD9,1,{0x1F}},
{0xC0,1,{0x40}},
{0xC1,1,{0x16}},
{0x80,1,{0x09}},
{0xFF,3,{0x98,0x82,0x08}},
{0xE0,27,{0x00,0x30,0x70,0xA0,0xDD,0x55,0x0D,0x32,0x5D,0x81,0xA5,0xB7,0xE2,0x09,0x2F,0xAA,0x57,0x89,0xA9,0xD2,0xFE,0xF7,0x26,0x60,0x90,0x03,0xEC}},
{0xE1,27,{0x00,0x18,0x70,0xA0,0xDD,0x55,0x0D,0x32,0x5D,0x81,0xA5,0xB7,0xE2,0x09,0x2F,0xAA,0x57,0x89,0xA9,0xD2,0xFE,0xF7,0x26,0x60,0x90,0x03,0xEC}},
{0xFF,3,{0x98,0x82,0x0B}},
{0x9A,1,{0x85}},
{0x9B,1,{0x93}},
{0x9C,1,{0x04}},
{0x9D,1,{0x04}},
{0x9E,1,{0x8B}},
{0x9F,1,{0x8B}},
{0xAA,1,{0x22}},
{0xAB,1,{0xE0}},
{0xAC,1,{0x7F}},
{0xAD,1,{0x3F}},
{0xFF,3,{0x98,0x82,0x0E}},
{0x11,1,{0x10}},
{0x13,1,{0x14}},
{0x00,1,{0xA0}},
{0xFF,3,{0x98,0x82,0x03}},
{0x8C,1,{0xD2}},
{0x8D,1,{0xD6}},
{0x8E,1,{0xDA}},
{0x8F,1,{0xDE}},
{0x90,1,{0xDF}},
{0x91,1,{0xE6}},
{0x92,1,{0xE9}},
{0x93,1,{0xED}},
{0x94,1,{0xF0}},
{0x95,1,{0xFF}},
{0x96,1,{0xB5}},
{0x97,1,{0xBA}},
{0x98,1,{0xBF}},
{0x99,1,{0xC4}},
{0x9A,1,{0xC9}},
{0x9B,1,{0xCD}},
{0x9C,1,{0xD5}},
{0x9D,1,{0xE6}},
{0x9E,1,{0xF6}},
{0x9F,1,{0xFB}},
{0x87,1,{0x4D}},
{0xFF,3,{0x98,0x82,0x06}},
{0x06,1,{0xA4}},
{0xFF,3,{0x98,0x82,0x03}},
{0x83,1,{0xB8}},
{0x84,1,{0x01}},
{0xFF,3,{0x98,0x82,0x00}},
//{0x51,2,{0x07,0xFF}},
{0x53,1,{0x2C}},
{0x55,1,{0x03}},
{0x68,1,{0x04}},
{0x68,1,{0x02}},
{0xFF,3,{0x98,0x82,0x00}},
{0x35,1,{0x00}},
{0x11,1,{0x00}},
{REGFLAG_DELAY, 100, {}},
{0x29,1,{0x00}},
{REGFLAG_DELAY, 20, {}}


};


static struct LCM_setting_table bl_level[] = {
	{0x51, 2, {0xFF,0xFF} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static void push_table_v2(void *cmdq, struct LCM_setting_table *table,
		       unsigned int count, unsigned char force_update)
{
	unsigned int i;
	unsigned int cmd;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;
		switch (cmd) {
		case REGFLAG_DELAY:
			if (table[i].count <= 10)
				MDELAY(table[i].count);
			else
				MDELAY(table[i].count);
			break;
		case REGFLAG_UDELAY:
			UDELAY(table[i].count);
			break;
		case REGFLAG_END_OF_TABLE:
			break;
		default:
			dsi_set_cmdq_V22(cmdq, cmd, table[i].count,
					 table[i].para_list, force_update);
			break;
		}
	}
}



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
#ifdef CONFIG_MTK_HIGH_FRAME_RATE
static void lcm_dfps_int(struct LCM_DSI_PARAMS *dsi)
{
	struct dfps_info *dfps_params = dsi->dfps_params;

	dsi->dfps_enable = 1;
	dsi->dfps_default_fps = 6000;/*real fps * 100, to support float*/
	dsi->dfps_def_vact_tim_fps = 9000;/*real vact timing fps * 100*/

	/*traversing array must less than DFPS_LEVELS*/
	/*DPFS_LEVEL0*/
	dfps_params[0].level = DFPS_LEVEL0;
	dfps_params[0].fps = 6000;/*real fps * 100, to support float*/
	dfps_params[0].vact_timing_fps = 9000;/*real vact timing fps * 100*/
	/*if mipi clock solution*/
	/*dfps_params[0].PLL_CLOCK = xx;*/
	/*dfps_params[0].data_rate = xx; */
	/*if HFP solution*/
	/*dfps_params[0].horizontal_frontporch = xx;*/
	dfps_params[0].vertical_frontporch = 1376;
	dfps_params[0].vertical_frontporch_for_low_power = 0;

	/*if need mipi hopping params add here
	dfps_params[0].dynamic_switch_mipi = 1;
	dfps_params[0].PLL_CLOCK_dyn = 550;
	dfps_params[0].horizontal_frontpo rch_dyn = 288;
	dfps_params[0].vertical_frontporch_dyn = 1291;
	dfps_params[0].vertical_frontporch_for_low_power_dyn = 2500;*/

	/*DPFS_LEVEL1*/
	dfps_params[1].level = DFPS_LEVEL1;
	dfps_params[1].fps = 9000;/*real fps * 100, to support float*/
	dfps_params[1].vact_timing_fps = 9000;/*real vact timing fps * 100*/
	/*if mipi clock solution*/
	/*dfps_params[1].PLL_CLOCK = xx;*/
	/*dfps_params[1].data_rate = xx; */
	/*if HFP solution*/
	/*dfps_params[1].horizontal_frontporch = xx;*/
	dfps_params[1].vertical_frontporch = 366;
	dfps_params[1].vertical_frontporch_for_low_power = 0;

	/*if need mipi hopping params add here
	dfps_params[1].dynamic_switch_mipi = 0;
	dfps_params[1].PLL_CLOCK_dyn = 550;
	dfps_params[1].horizontal_frontporch_dyn = 288;
	dfps_params[1].vertical_frontporch_dyn = 54;
	dfps_params[1].vertical_frontporch_for_low_power_dyn = 2500;*/

	dsi->dfps_num = 2;
}
#endif


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
	params->dsi.switch_mode = BURST_VDO_MODE;
	lcm_dsi_mode = CMD_MODE;
    #else
	params->dsi.mode   = BURST_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; 
	params->dsi.switch_mode = CMD_MODE;
	lcm_dsi_mode = BURST_VDO_MODE;
    #endif
	LCM_DBG("%s lcm_dsi_mode %d\n", __func__, lcm_dsi_mode);
	params->dsi.switch_mode_enable = 0;
	
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
    params->dsi.vertical_backporch              = 24;
    params->dsi.vertical_frontporch             = 1376;
	params->dsi.vertical_active_line = FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active              = 16;
    params->dsi.horizontal_backporch                = 16;
    params->dsi.horizontal_frontporch               = 16; 
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
    params->dsi.compatibility_for_nvk = 0;
    params->dsi.ssc_disable=1;
    params->dsi.ssc_range=2;

#if (LCM_DSI_CMD_MODE)
	/* this value must be in MTK suggested table */
	params->dsi.PLL_CLOCK = 456;
#else
	/* this value must be in MTK suggested table */
	params->dsi.PLL_CLOCK = 456;
#endif
		

	params->dsi.esd_check_enable = 1; 
	params->dsi.customization_esd_check_enable = 1;//0:te esd check 1:read register
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;

	//params->dsi.lcm_esd_check_table[1].cmd = 0x0D;
	//params->dsi.lcm_esd_check_table[1].count = 1;
//	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x00;
#ifdef CONFIG_MTK_HIGH_FRAME_RATE
	/****DynFPS start****/
	lcm_dfps_int(&(params->dsi));
	/****DynFPS end****/
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
	MDELAY(10);
    mt_set_gpio_out(gpio_bl_enn,GPIO_OUT_ONE);
    MDELAY(10);
#else //Kernel driver
	_lcm_i2c_write_bytes(0x00,0x14);
    MDELAY(1);
    _lcm_i2c_write_bytes(0x01,0x14);
    MDELAY(1);
    _lcm_i2c_write_bytes(0xFF,0x80);
    MDELAY(1);
    lcm_enp(1);
	MDELAY(10);
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
    MDELAY(50);
    LCM_DBG("debug,lcm reset end \n");
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
    LCM_DBG("debug,lcm init end \n");
}

static void lcm_suspend(void)
{
    LCM_DBG();
#ifndef BUILD_LK
    push_table(lcm_deep_sleep_mode_in_setting_v2, sizeof(lcm_deep_sleep_mode_in_setting_v2) / sizeof(struct LCM_setting_table), 1);

	MDELAY(10);
	lcm_enn(0);
    MDELAY(3);
    lcm_enp(0);
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

static int BRIGHT_MAP[256]={
0,5,
13,23,34,41,
47,53,60,66,71,76,82,
88,95,101,108,114,121,127,134,140,147,153,160,166,173,179,186,193,199,205,
211,217,224,230,236,243,249,255,262,268,274,280,287,
293,300,307,314,320,327,334,341,348,
356,362,368,375,381,387,393,400,406,412,418,425,431,437,444,450,
456,462,468,475,481,487,493,500,506,512,518,525,532,
538,544,550,557,563,569,575,582,588,594,600,607,614,
620,626,633,639,645,652,658,664,671,677,683,690,696,702,709,715,721,728,737,
743,749,756,762,768,775,781,787,794,800,806,813,819,825,832,838,845,853,860,
867,874,881,888,895,901,
908,915,921,
927,933,940,946,953,959,965,972,978,985,991,997,1004,1010,1017,1023,1029,1036,1042,1049,1055,1061,1068,1074,1081,1087,1093,1100,1106,1113,1119,1125,1132,1138,1145,1151,1157,1164,1170,1177,1183,1189,1196,1202,1209,1215,1221,1228,1234,1241,1247,1253,1260,1266,1273,1279,1285,1292,1298,1305,1311,1317,1324,1330,1337,1343,1349,1356,1362,1369,1375,1381,1388,1394,1401,1407,1413,1420,1426,1433,
1439,1445,1452,1458,1465,1471,1477,1484,1490,1497,1503,1509,1516,1522,1529,1535,1541,1548,1554,1561,1567,1573,1580,1586,1593,1599,1605,1612,1618,1625,1631,1638,
2047};

static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{
	unsigned int BacklightLevel;

	if (level > 255){
        level = 255;
		}
    if (level < 0){
        level = 0;
        }

	BacklightLevel =  BRIGHT_MAP[level];
	LCM_DBG("%s,icili9882q backlight: BacklightLevel = %d\n", __func__, BacklightLevel);
    LCM_DBG("%s,icili9882q backlight: level = %d\n", __func__, level);
	bl_level[0].para_list[0] = ((BacklightLevel >> 8) & 0x7);
	bl_level[0].para_list[1] = (BacklightLevel & 0xff);
	
	push_table_v2(handle, bl_level, ARRAY_SIZE(bl_level), 1);
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

struct LCM_DRIVER icili9882q_p410_hdplus_dsi_vdo_txd_lcm_drv = 
{
	.name		= "icili9882q_p410_hdplus_dsi_vdo_txd",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
	.set_backlight_cmdq  = lcm_setbacklight_cmdq,
};
