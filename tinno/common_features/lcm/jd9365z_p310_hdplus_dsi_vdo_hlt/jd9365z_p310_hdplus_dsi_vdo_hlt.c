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

//#include <mt-plat/mtk_gpio.h>
#endif



// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define REGFLAG_DELAY                                       0XFE
#define REGFLAG_END_OF_TABLE                                0xDD   // END OF REGISTERS MARKER
#define LCM_ID       (0x9365)


#define LCM_DSI_CMD_MODE                                    0

#ifndef TRUE
#define   TRUE     1
#endif

#ifndef FALSE
#define   FALSE    0
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
    LCM_PRINT("[jd9365z_p310_hdplus_dsi_vdo_hlt.c kernel] %s (line:%d) :" fmt "\r\n", __func__, __LINE__, ## arg)

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (720)
#define FRAME_HEIGHT (1520)
#define VIRTUAL_WIDTH	(1080)
#define VIRTUAL_HEIGHT	(1920)

/* physical size in um */
#define LCM_PHYSICAL_WIDTH (68400)
#define LCM_PHYSICAL_HEIGHT (143640)
#define LCM_DENSITY	(320)
//static struct LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static  struct LCM_UTIL_FUNCS lcm_util ;

#define SET_RESET_PIN(v)                                    (lcm_util.set_reset_pin((v)))

#define UDELAY(n)                                           (lcm_util.udelay(n))
#define MDELAY(n)                                           (lcm_util.mdelay(n))

static unsigned int lcm_compare_id(void);
// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                      lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                  lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)                                       lcm_util.dsi_read_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

extern void lcm_enn(int onoff);
extern void lcm_enp(int onoff);
extern  int _lcm_i2c_write_bytes(unsigned char addr, unsigned char value);
extern int get_lcm_id_status(void);

struct LCM_setting_table
{
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] =
{
	{0xE0,1,{0x00}},

	{0xE1,1,{0x93}},
	{0xE2,1,{0x65}},
	{0xE3,1,{0xF8}},
	{0x80,1,{0x03}},


	{0xE0,1,{0x01}},

	{0x00,1,{0x00}},
	{0x01,1,{0x35}},
	{0x03,1,{0x00}},
	{0x04,1,{0x35}},

	{0x17,1,{0x00}},
	{0x18,1,{0xD7}}, 
	{0x19,1,{0x00}}, 
	{0x1A,1,{0x00}}, 
	{0x1B,1,{0xD7}}, 
	{0x1C,1,{0x00}},

	{0x1F,1,{0x6B}}, 
	{0x20,1,{0x24}}, 
	{0x21,1,{0x24}},
	{0x22,1,{0x4E}},
	{0x24,1,{0xFE}},

	{0x37,1,{0x09}},	

	{0x38,1,{0x04}},	
	{0x39,1,{0x08}},	
	{0x3A,1,{0x12}},	
	{0x3C,1,{0x64}},	
	{0x3D,1,{0xFF}},
	{0x3E,1,{0xFF}},
	{0x3F,1,{0x64}},

	{0x40,1,{0x04}},	
	{0x41,1,{0xBE}},	
	{0x42,1,{0x6B}},	
	{0x43,1,{0x12}},	
	{0x44,1,{0x0F}},	
	{0x45,1,{0x28}},


	{0x55,1,{0x0F}},
	{0x56,1,{0x01}},
	{0x57,1,{0x69}},
	{0x58,1,{0x0A}},
	{0x59,1,{0x0A}},	
	{0x5A,1,{0x29}},	
	{0x5B,1,{0x10}},	

	{0x5D,1,{0x6F}},//
	{0x5E,1,{0x3D}},//
	{0x5F,1,{0x2D}},//
	{0x60,1,{0x22}},//
	{0x61,1,{0x20}},//
	{0x62,1,{0x14}},//
	{0x63,1,{0x1C}},//
	{0x64,1,{0x09}},//
	{0x65,1,{0x27}},//
	{0x66,1,{0x29}},//
	{0x67,1,{0x2C}},//
	{0x68,1,{0x4D}},//
	{0x69,1,{0x3E}},//
	{0x6A,1,{0x49}},//
	{0x6B,1,{0x3C}},//
	{0x6C,1,{0x3B}},//
	{0x6D,1,{0x2F}},//
	{0x6E,1,{0x1F}},//
	{0x6F,1,{0x09}},//
	{0x70,1,{0x6F}},//
	{0x71,1,{0x3D}},//
	{0x72,1,{0x2D}},//
	{0x73,1,{0x22}},//
	{0x74,1,{0x20}},//
	{0x75,1,{0x14}},//
	{0x76,1,{0x1C}},//
	{0x77,1,{0x09}},//
	{0x78,1,{0x27}},//
	{0x79,1,{0x29}},//
	{0x7A,1,{0x2C}},//
	{0x7B,1,{0x4D}},//
	{0x7C,1,{0x3E}},//
	{0x7D,1,{0x49}},//
	{0x7E,1,{0x3C}},//
	{0x7F,1,{0x3B}},//
	{0x80,1,{0x2F}},//
	{0x81,1,{0x1F}},//
	{0x82,1,{0x09}},//


	{0xE0,1,{0x02}},

	{0x00,1,{0x5E}},	
	{0x01,1,{0x5F}}, 
	{0x02,1,{0x57}},  
	{0x03,1,{0x58}},  
	{0x04,1,{0x44}},  
	{0x05,1,{0x46}},  
	{0x06,1,{0x48}},  
	{0x07,1,{0x4A}},  
	{0x08,1,{0x40}},  
	{0x09,1,{0x1D}}, 
	{0x0A,1,{0x1D}},  
	{0x0B,1,{0x1D}},  
	{0x0C,1,{0x1D}},  
	{0x0D,1,{0x1D}},  
	{0x0E,1,{0x1D}},  
	{0x0F,1,{0x50}},  
	{0x10,1,{0x5F}},  
	{0x11,1,{0x5F}},  
	{0x12,1,{0x5F}},  
	{0x13,1,{0x5F}},  
	{0x14,1,{0x5F}}, 
	{0x15,1,{0x5F}},  

	{0x16,1,{0x5E}},  
	{0x17,1,{0x5F}},  
	{0x18,1,{0x57}},  
	{0x19,1,{0x58}},  
	{0x1A,1,{0x45}},  
	{0x1B,1,{0x47}},  
	{0x1C,1,{0x49}},  
	{0x1D,1,{0x4B}},  
	{0x1E,1,{0x41}},  
	{0x1F,1,{0x1D}},  
	{0x20,1,{0x1D}},  
	{0x21,1,{0x1D}},  
	{0x22,1,{0x1D}},  
	{0x23,1,{0x1D}},  
	{0x24,1,{0x1D}},  
	{0x25,1,{0x51}},  
	{0x26,1,{0x5F}},  
	{0x27,1,{0x5F}},  
	{0x28,1,{0x5F}},  
	{0x29,1,{0x5F}},  
	{0x2A,1,{0x5F}},  
	{0x2B,1,{0x5F}},  

	{0x2C,1,{0x1F}},
	{0x2D,1,{0x1E}},
	{0x2E,1,{0x17}},
	{0x2F,1,{0x18}},
	{0x30,1,{0x0B}},
	{0x31,1,{0x09}},
	{0x32,1,{0x07}},
	{0x33,1,{0x05}},
	{0x34,1,{0x11}},
	{0x35,1,{0x1F}},
	{0x36,1,{0x1F}},
	{0x37,1,{0x1F}},
	{0x38,1,{0x1F}},
	{0x39,1,{0x1F}},
	{0x3A,1,{0x1F}},
	{0x3B,1,{0x01}},
	{0x3C,1,{0x1F}},
	{0x3D,1,{0x1F}},
	{0x3E,1,{0x1F}},
	{0x3F,1,{0x1F}},
	{0x40,1,{0x1F}},
	{0x41,1,{0x1F}},

	{0x42,1,{0x1F}},
	{0x43,1,{0x1E}},
	{0x44,1,{0x17}},
	{0x45,1,{0x18}},
	{0x46,1,{0x0A}},
	{0x47,1,{0x08}},
	{0x48,1,{0x06}},
	{0x49,1,{0x04}},
	{0x4A,1,{0x10}},
	{0x4B,1,{0x1F}},
	{0x4C,1,{0x1F}},
	{0x4D,1,{0x1F}},
	{0x4E,1,{0x1F}},
	{0x4F,1,{0x1F}},
	{0x50,1,{0x1F}},
	{0x51,1,{0x00}},
	{0x52,1,{0x1F}},
	{0x53,1,{0x1F}},
	{0x54,1,{0x1F}},
	{0x55,1,{0x1F}},
	{0x56,1,{0x1F}},
	{0x57,1,{0x1F}},

	{0x58,1,{0x40}},
	{0x59,1,{0x00}},
	{0x5A,1,{0x00}},
	{0x5B,1,{0x10}},
	{0x5C,1,{0x0B}},
	{0x5D,1,{0x30}},
	{0x5E,1,{0x01}},
	{0x5F,1,{0x02}},
	{0x60,1,{0x30}},
	{0x61,1,{0x03}},
	{0x62,1,{0x04}},
	{0x63,1,{0x1C}},
	{0x64,1,{0x52}}, 
	{0x65,1,{0x56}},
	{0x66,1,{0x00}},
	{0x67,1,{0x73}},
	{0x68,1,{0x0D}},
	{0x69,1,{0x0D}},
	{0x6A,1,{0x52}}, 
	{0x6B,1,{0x00}},
	{0x6C,1,{0x00}},
	{0x6D,1,{0x00}},
	{0x6E,1,{0x00}},
	{0x6F,1,{0x88}},
	{0x70,1,{0x00}},
	{0x71,1,{0x00}},
	{0x72,1,{0x06}},
	{0x73,1,{0x7B}},
	{0x74,1,{0x00}},
	{0x75,1,{0xBC}},
	{0x76,1,{0x00}},
	{0x77,1,{0x0E}},
	{0x78,1,{0x11}},
	{0x79,1,{0x00}},
	{0x7A,1,{0x00}},
	{0x7B,1,{0x00}},
	{0x7C,1,{0x00}},
	{0x7D,1,{0x03}},
	{0x7E,1,{0x7B}},


	{0xE0,1,{0x04}},
	{0x09,1,{0x11}},
	{0x0E,1,{0x4A}},


	{0xE0,1,{0x00}},

	{0x11, 1, {0x00}}, 	// SLPOUT
	{REGFLAG_DELAY,120,{}},

	{0x29,1,{0x00}},  	// DSPON
	{REGFLAG_DELAY,5,{}},

	{0x35,1,{0x00}},



	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] =
{
    // Sleep Mode On
    // Display off sequence
    {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 50, {}},

    // Sleep Mode On
    {0x10, 1, {0x00}},
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


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

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
		params->dsi.mode   = SYNC_PULSE_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; 
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
    params->dsi.vertical_backporch              = 18;
    params->dsi.vertical_frontporch             = 17;
		params->dsi.vertical_active_line = FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active              = 4;
    params->dsi.horizontal_backporch                = 44;
    params->dsi.horizontal_frontporch               = 44; 
		params->dsi.horizontal_active_pixel = FRAME_WIDTH;
    params->dsi.compatibility_for_nvk = 0;
    params->dsi.ssc_disable=1;
    params->dsi.ssc_range=2;
    params->dsi.PLL_CLOCK                = 241;

    params->dsi.esd_check_enable = 1;
    params->dsi.customization_esd_check_enable = 1; //0:te esd check 1:read register
    params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
    params->dsi.lcm_esd_check_table[0].count        = 1;
    params->dsi.lcm_esd_check_table[0].para_list[0] = 0x1C;
#ifdef CONFIG_MTK_ROUND_CORNER_SUPPORT
	params->round_corner_en = 1;
	params->full_content = 0;
	params->corner_pattern_width = 32;
	params->corner_pattern_height = 32;
	params->corner_pattern_height_bot = 20;
#endif

}


static void lcm_init(void)
{
    LCM_DBG();
#ifdef BUILD_LK
    mt_set_gpio_mode(gpio_bl_enp,GPIO_MODE_00);
    mt_set_gpio_dir(gpio_bl_enp,GPIO_DIR_OUT);
    mt_set_gpio_mode(gpio_bl_enn,GPIO_MODE_00);
    mt_set_gpio_dir(gpio_bl_enn,GPIO_DIR_OUT);
    mt_set_gpio_out(gpio_bl_enp,GPIO_OUT_ONE);
    MDELAY(1);
    lcd_write_byte(0,0x12);
    MDELAY(1);
    lcd_write_byte(1,0x12);
    MDELAY(1);
    mt_set_gpio_out(gpio_bl_enn,GPIO_OUT_ONE);
#else //Kernel driver
    lcm_enp(1);
    MDELAY(3);
    _lcm_i2c_write_bytes(0,0x12);//5.8
    MDELAY(3);
    _lcm_i2c_write_bytes(1,0x12);
    MDELAY(3);
    lcm_enn(1);
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
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
   // LCM_DBG();
#ifndef BUILD_LK
    //LCM_DBG("geroge suspend bEnTGesture  = %d\n",bEnTGesture);

    push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);

        //SET_RESET_PIN(1);
        //SET_RESET_PIN(0);
       // MDELAY(10); // 1ms


        lcm_enp(0);
        MDELAY(1);
        lcm_enn(0);
        MDELAY(1);
        //SET_RESET_PIN(1);
        //MDELAY(50);


#endif

}

static void lcm_resume(void)
{

#ifndef BUILD_LK
    //LCM_DBG("geroge resume bEnTGesture  = %d\n",bEnTGesture);
        lcm_enp(1);
        MDELAY(1);
        _lcm_i2c_write_bytes(0,0x0F);
        MDELAY(1);
        _lcm_i2c_write_bytes(1,0x0F);
        MDELAY(1);
        lcm_enn(1);
#endif
    MDELAY(1);
    SET_RESET_PIN(0);
    MDELAY(5);
    SET_RESET_PIN(1);
    MDELAY(10);
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
    LCM_DBG();
}
static unsigned int lcm_compare_id(void)
{
    int array[4];
    char buffer[5];
    char id_high=0;
    char id_midd=0;
    char id_low=0;
    int id=0;

    //Do reset here
    SET_RESET_PIN(1);
    MDELAY(2);
    SET_RESET_PIN(0);
    MDELAY(25);
    SET_RESET_PIN(1);
    MDELAY(120);

    array[0]=0x00023700;//0x00023700;
    dsi_set_cmdq(array, 1, 1);
    //read_reg_v2(0x04, buffer, 3);//if read 0x04,should get 0x008000,that is both OK.

    read_reg_v2(0xDA, buffer,1);
    id_high = buffer[0]; ///////////////////////0x98
    //LCM_DBG("JD9161:  id_high =%x\n", id_high);
    MDELAY(2);
    read_reg_v2(0xDB, buffer,1);
    id_midd = buffer[0]; ///////////////////////0x06
    //LCM_DBG("JD9161:  id_midd =%x\n", id_midd);
    MDELAY(2);
    read_reg_v2(0xDC, buffer,1);
    id_low = buffer[0]; ////////////////////////0x04
    //LCM_DBG("JD9161:  id_low =%x\n", id_low);

    id = (id_high << 8) | id_midd;
    LCM_DBG("id = %x\n", id);

    return (LCM_ID == id)?1:0;
}

struct LCM_DRIVER jd9365z_p310_hdplus_dsi_vdo_hlt_lcm_drv =
{
    .name           = "jd9365z_p310_hdplus_dsi_vdo_hlt",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id    = lcm_compare_id,
#if (LCM_DSI_CMD_MODE)
    .set_backlight  = lcm_setbacklight,
    .update         = lcm_update,
#endif
};
