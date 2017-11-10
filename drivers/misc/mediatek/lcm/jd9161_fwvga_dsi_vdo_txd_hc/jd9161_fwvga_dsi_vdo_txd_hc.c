#include <linux/string.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <asm-generic/gpio.h>

#include "lcm_drv.h"
#include "ddp_irq.h"

static unsigned int GPIO_LCD_RST;




/* --------------------------------------------------------------------------- */
/* Local Constants */
/* --------------------------------------------------------------------------- */

// yewei add in 20160727
//#define FRAME_WIDTH  (540)
//#define FRAME_HEIGHT (960)
#define FRAME_WIDTH  (480)
#define FRAME_HEIGHT (854)
// yewei add in 20160727

/* --------------------------------------------------------------------------- */
/* Local Variables */
/* --------------------------------------------------------------------------- */

static LCM_UTIL_FUNCS lcm_util = { 0 };

//#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))
#define REGFLAG_DELAY								0XFD
#define REGFLAG_END_OF_TABLE							0xFE	/* END OF REGISTERS MARKER */


/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)									lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) lcm_util.dsi_write_regs(addr, pdata, byte_nums)
/* #define read_reg lcm_util.dsi_read_reg() */
#define read_reg_v2(cmd, buffer, buffer_size) lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

extern int IMM_GetOneChannelValue_Cali(int Channel, int *voltage);

/* #define LCM_DSI_CMD_MODE */

struct LCM_setting_table {
	unsigned cmd;
	unsigned char count;
	unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {

	/*
	   Note :

	   Data ID will depends on the following rule.

	   count of parameters > 1      => Data ID = 0x39
	   count of parameters = 1      => Data ID = 0x15
	   count of parameters = 0      => Data ID = 0x05

	   Structure Format :

	   {DCS command, count of parameters, {parameter list}}
	   {REGFLAG_DELAY, milliseconds of time, {}},

	   ...

	   Setting ending by predefined flag

	   {REGFLAG_END_OF_TABLE, 0x00, {}}
	 */
// yewei add in 20160727
	{0xBF,03,{0x91,0x61,0xF2}},

	{0xB3,02,{0x00,0x6C}},

	{0xB4,02,{0x00,0x73}},

	{0xB8,06,{0x00,0xB6,0x01,0x00,0xB6,0x01}},

	{0xBA,03,{0x34,0x23,0x00}},

	{0xC3,01,{0x02}},

	{0xC4,02,{0x30,0x6A}},
	//{0xC4,8,{0x30,0x6A,0x84,0x0E,0x09,0x04,0x00,0xD0}}, //bist mode

	{0xC7,9,{0x00,0x01,0x31,0x05,0x65,0x2C,0x13,0xA5,0xA5}},

	{0xC8,38,{0x79,0x75,0x63,0x4F,0x3C,0x27,0x26,0x10,0x2B,0x2E,0x32,0x55,0x4A,0x5B,0x55,0x5C,0x57,0x4F,0x41,0x79,0x75,0x63,0x4F,0x3C,0x27,0x26,0x10,0x2B,0x2E,0x32,0x55,0x4A,0x5B,0x55,0x5C,0x57,0x4F,0x41}},

	{0xD4,16,{0x1F,0x1E,0x1F,0x00,0x10,0x1F,0x1F,0x04,0x08,0x06,0x0A,0x1F,0x1F,0x1F,0x1F,0x1F}},

	{0xD5,16,{0x1F,0x1E,0x1F,0x01,0x11,0x1F,0x1F,0x05,0x09,0x07,0x0B,0x1F,0x1F,0x1F,0x1F,0x1F}},

	{0xD6,16,{0x1F,0x1F,0x1E,0x11,0x01,0x1F,0x1F,0x09,0x05,0x07,0x0B,0x1F,0x1F,0x1F,0x1F,0x1F}},

	{0xD7,16,{0x1F,0x1F,0x1E,0x10,0x00,0x1F,0x1F,0x08,0x04,0x06,0x0A,0x1F,0x1F,0x1F,0x1F,0x1F}},

	{0xD8,20,{0x20,0x02,0x0A,0x10,0x05,0x30,0x01,0x02,0x30,0x01,0x02,0x06,0x70,0x53,0x61,0x73,0x09,0x06,0x70,0x08}},

	{0xD9,19,{0x00,0x0A,0x0A,0x88,0x00,0x00,0x06,0x7B,0x00,0x00,0x00,0x3B,0x2F,0x1F,0x00,0x00,0x00,0x03,0x7B}},

	{0xBE,01,{0x01}},

	{0xC1,01,{0x10}},

	{0xCC,10,{0x34,0x20,0x38,0x60,0x11,0x91,0x00,0x40,0x00,0x00}},

	{0xBE,01,{0x00}},

	{0x11,01,{0x00}},

	{REGFLAG_DELAY, 120, {}},

	{0x29,01,{0x00}},
	//{0x28,01,{0x00}}, //bist mode
	{REGFLAG_DELAY, 10, {}},

	{0xBF,03,{0x09,0xB1,0x7F}},
	{REGFLAG_DELAY, 2, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
//yewei add in 20160727
};

#if 0
static struct LCM_setting_table lcm_sleep_out_setting[] = {
	/* Sleep Out */
	{ 0x11, 1, {0x00} },
	{ REGFLAG_DELAY, 20, {} },

	/* Display ON */
	{ 0x29, 1, {0x00} },
	{ REGFLAG_DELAY, 120, {} },
	{ REGFLAG_END_OF_TABLE, 0x00, {} }
};
#endif
 

void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
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
static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	gpio_direction_output(GPIO, output);
	gpio_set_value(GPIO, output);
}


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

	params->physical_width = 62; //62.06mm
	params->physical_height = 110; //110.42mm

	/* enable tearing-free */
	params->dbi.te_mode = LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity = LCM_POLARITY_RISING;

#if defined(LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
#endif

	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_TWO_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	/* Not support in MT6573 */

	params->dsi.DSI_WMEM_CONTI = 0x3C;
	params->dsi.DSI_RMEM_CONTI = 0x3E;


	params->dsi.packet_size = 256;

	/* Video mode setting */
	params->dsi.intermediat_buffer_num = 2;

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

#if 1
	params->dsi.vertical_sync_active = 2;//3;
	params->dsi.vertical_backporch = 10;//12;
	params->dsi.vertical_frontporch = 10;//6 10;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 10;
	params->dsi.horizontal_backporch = 62;//63
	params->dsi.horizontal_frontporch = 62;//63
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	/* Bit rate calculation */
	params->dsi.PLL_CLOCK = 200;//265;
#else
	params->dsi.vertical_sync_active = 3;
	params->dsi.vertical_backporch = 12;
	params->dsi.vertical_frontporch = 3;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 10;
	params->dsi.horizontal_backporch = 63;
	params->dsi.horizontal_frontporch = 63;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	/* Bit rate calculation */
	params->dsi.PLL_CLOCK = 252;
#endif

	params->dsi.clk_lp_per_line_enable = 1;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	//params->dsi.clk_lp_per_line_enable = 0; //bist mode
	//params->dsi.esd_check_enable = 0; //bist mode
	//params->dsi.customization_esd_check_enable = 0; //bist mode
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
	params->dsi.noncont_clock = 1; //remove on bist mode
	params->dsi.noncont_clock_period = 2; // Unit : frames //remove on bist mode
}
static int get_lcd_pin_id_vol(void)
{
    int res = 0;
    int lcm_vol = 0;

    res = IMM_GetOneChannelValue_Cali(12,&lcm_vol);
    if(res < 0)
    {
		printk("jd9161,[adc_uboot]: get data error\n");
		return 0;
    }

    printk("jd9161,[adc_uboot]: lcm_vol= %d\n",lcm_vol);
	lcm_vol = lcm_vol/1000;
    if (lcm_vol <= 200)
    {
		return 1;
    }
    else
		return 0;
}

static unsigned int lcm_compare_id(void)
{
	int array[4];
	char buffer[3];
	char id0 = 0;
	char id1 = 0;
	char id2 = 0;

	lcm_set_gpio_output(GPIO_LCD_RST, 0);
	//SET_RESET_PIN(0);
	MDELAY(200);
	lcm_set_gpio_output(GPIO_LCD_RST, 1);
	//SET_RESET_PIN(1);
	MDELAY(200);

	array[0] = 0x00033700;	/* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xDA, buffer, 1);


	array[0] = 0x00033700;	/* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0xDB, buffer + 1, 1);


	array[0] = 0x00033700;	/* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0xDC, buffer + 2, 1);

	id0 = buffer[0];	/* should be 0x91 */
	id1 = buffer[1];	/* should be 0x61 */
	id2 = buffer[2];	/* should be 0x00 */
#ifdef BUILD_LK
	printf("%s, id0 = 0x%08x\n", __func__, id0);	/* should be 0x91 */
	printf("%s, id1 = 0x%08x\n", __func__, id1);	/* should be 0x61 */
	printf("%s, id2 = 0x%08x\n", __func__, id2);	/* should be 0x00 */
#endif
	if((id0==0x91)&&(id1==0x61)&&(id2==0x00))
		return get_lcd_pin_id_vol();
	else
		return 0;
}
static void lcm_get_gpio_infor(void)
{
	static struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "mediatek,DISPSYS");

	GPIO_LCD_RST = of_get_named_gpio(node, "lcm_power_rst_gpio", 0);

	//printk("csq %x, %x, %x\n", GPIO_LCD_PWR_ENP, GPIO_LCD_PWR_ENN, GPIO_LCD_RST);
}
static void lcm_init(void)
{
	lcm_get_gpio_infor();

	lcm_set_gpio_output(GPIO_LCD_RST, 1);
	//SET_RESET_PIN(1);
	MDELAY(10);
	lcm_set_gpio_output(GPIO_LCD_RST, 0);
	//SET_RESET_PIN(0);
	MDELAY(10);
	lcm_set_gpio_output(GPIO_LCD_RST, 1);
	//SET_RESET_PIN(1);
	MDELAY(120);
	push_table(lcm_initialization_setting,
		   sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}



static void lcm_suspend(void)
{
	unsigned int data_array[2];
	MDELAY(10);
	data_array[0]=0x00043902;
	data_array[1]=0xF26191BF;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	data_array[0] = 0x00280500;	/* Display Off */
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10);
	data_array[0] = 0x00100500;	/* Sleep In */
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
	lcm_set_gpio_output(GPIO_LCD_RST, 0);
	//SET_RESET_PIN(0);
	MDELAY(50);
#ifdef BUILD_LK
	printf("uboot %s\n", __func__);
#else
	pr_debug("kernel %s\n", __func__);
#endif
}


static void lcm_resume(void)
{
#ifdef BUILD_LK
	printf("uboot %s\n", __func__);
#else
	pr_debug("kernel %s\n", __func__);
#endif
/* push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1); */
	lcm_init();
}

#ifdef LCM_DSI_CMD_MODE
static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);
	unsigned char y0_MSB = ((y0 >> 8) & 0xFF);
	unsigned char y0_LSB = (y0 & 0xFF);
	unsigned char y1_MSB = ((y1 >> 8) & 0xFF);
	unsigned char y1_LSB = (y1 & 0xFF);

	unsigned int data_array[16];

#ifdef BUILD_LK
	printf("uboot %s\n", __func__);
#else
	pr_debug("kernel %s\n", __func__);
#endif

	data_array[0] = 0x00053902;
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	data_array[3] = 0x00053902;
	data_array[4] = (y1_MSB << 24) | (y0_LSB << 16) | (y0_MSB << 8) | 0x2b;
	data_array[5] = (y1_LSB);
	data_array[6] = 0x002c3909;

	dsi_set_cmdq(data_array, 7, 0);

}
#endif

/*
#ifdef CONFIG_WT_BRIGHTNESS_MAPPING_WITH_LCM
static int bl_Cust_Max = 1023;//zhengzhou.wt modify for 380nit
static int bl_Cust_Min = 16;

static int setting_max = 1023;
static int setting_min = 28;
static int lcm_brightness_mapping(int level)
{
   int mapped_level;
   if(level >= setting_min){
	 mapped_level = ((bl_Cust_Max-bl_Cust_Min)*level+(setting_max*bl_Cust_Min)-(setting_min*bl_Cust_Max))/(setting_max-setting_min);
   }else{
	 mapped_level = (bl_Cust_Min*level)/setting_min;
   } 
   #ifdef BUILD_LK
   printf("level= %d, lcm_brightness_mapping= %d\n", level, mapped_level);
   #else
   printk("level= %d, lcm_brightness_mapping= %d\n", level, mapped_level);
   #endif
   return mapped_level;
}
#endif
*/

LCM_DRIVER jd9161_fwvga_dsi_vdo_txd_hc_lcm_drv = {

	.name = "jd9161_fwvga_dsi_vdo_txd_hc",
	.set_util_funcs = lcm_set_util_funcs,
	.compare_id = lcm_compare_id,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
/*.cust_mapping = lcm_brightness_mapping,*/ // yewei.wt,add,20160812.
	.resume = lcm_resume,
#if defined(LCM_DSI_CMD_MODE)
	.update = lcm_update,
#endif
};
