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


	{0XFF, 5, {0XFF, 0X98, 0X06, 0X04, 0X01}},	//EXTC Command Set enable register (Page 1)
	{0X08, 1, {0X10}},	//Interface Mode Control 1
	{0X21, 1, {0X01}},	//Display Function Control 2
	{0X30, 1, {0X01}},	//Resolution Control (480*854)
	{0X31, 1, {0X02}},	//Display Inversion Control
	{0X40, 1, {0X10}},	//Display Inversion Control //14 15, BT   10
	{0X41, 1, {0X33}},	//Display Inversion Control //33 44, DVDDH DVDDL clamp
	{0X42, 1, {0X03}},	//Display Inversion Control
	{0X43, 1, {0X09}},	//Display Inversion Control
	{0X44, 1, {0X09}},	//Display Inversion Control
	//{0X46, 1, {0X44}},	//Display Inversion Control
	{0X50, 1, {0X70}},	//Display Inversion Control //78 88 VGMP
	{0X51, 1, {0X70}},	//Display Inversion Control //78 88 VGMN
	{0X52, 1, {0X00}},	//Display Inversion Control
	{0X53, 1, {0X44}},	//Display Inversion Control //28 3D Flicker 1207
	{0X56, 1, {0X01}},	//Display Inversion Control
	{0X57, 1, {0X50}},	//Display Inversion Control
	{0X60, 1, {0X0A}},	//Source Timing Adjust1 (SDTI) //SDTI 0A
	{0X61, 1, {0X00}},	//Source Timing Adjust2 (CRTI)
	{0X62, 1, {0X08}},	//Source Timing Adjust3 (EQTI)
	{0X63, 1, {0X00}},	//Source Timing Adjust4 (EQTI)
	{0XA0, 1, {0X00}},	//Positive Gamma Control (01h~16h)
	{0XA1, 1, {0X03}},	//02 05 Gamma 4  越小越暗
	{0XA2, 1, {0X0A}},	//0D 0F Gamma 8
	{0XA3, 1, {0X0E}},	//11 14 Gamma 16
	{0XA4, 1, {0X09}},	//0A 0D Gamma 24
	{0XA5, 1, {0X18}},	//19 1E Gamma 52
	{0XA6, 1, {0X0B}},	//08 0F Gamma 80
	{0XA7, 1, {0X0B}},	//06 0C Gamma 108
	{0XA8, 1, {0X02}},	//01 03 Gamma 147
	{0XA9, 1, {0X08}},	//06 08 Gamma 175
	{0XAA, 1, {0X04}},	//02 0B Gamma 203
	{0XAB, 1, {0X03}},	//04 06 Gamma 231
	{0XAC, 1, {0X0B}},	//0B 12 Gamma 239
	{0XAD, 1, {0X2F}},	//29 33 Gamma 247
	{0XAE, 1, {0X2B}},	//26 33 Gamma 251  越小越亮
	{0XAF, 1, {0X00}},
	{0XC0, 1, {0X00}},	//Negative Gamma Control (01h~16h)
	{0XC1, 1, {0X03}},	//02 05 Gamma 4
	{0XC2, 1, {0X09}},	//11 0C Gamma 8
	{0XC3, 1, {0X0E}},	//0E 11 Gamma 16
	{0XC4, 1, {0X08}},	//07 0B Gamma 24
	{0XC5, 1, {0X18}},	//17 1D Gamma 52
	{0XC6, 1, {0X0B}},	//0D 0C Gamma 80
	{0XC7, 1, {0X0A}},	// Gamma 108
	{0XC8, 1, {0X02}},	//02 03 Gamma 147
	{0XC9, 1, {0X08}},	//06 08 Gamma 175
	{0XCA, 1, {0X05}},	//00 06 Gamma 203
	{0XCB, 1, {0X04}},	//04 05 Gamma 231
	{0XCC, 1, {0X0A}},	//07 0E Gamma 239
	{0XCD, 1, {0X2E}},	//26 28 Gamma 247
	{0XCE, 1, {0X2A}},	//24 1F Gamma 251
	{0XCF, 1, {0X00}},
	{0XFF, 5, {0XFF, 0X98, 0X06, 0X04, 0X06}},	//Change to page 7
	{0X00, 1, {0X21}},	//GIP Setting (00h~40h)
	{0X01, 1, {0X09}},	//0A
	{0X02, 1, {0X00}},
	{0X03, 1, {0X00}},
	{0X04, 1, {0X01}},
	{0X05, 1, {0X01}},
	{0X06, 1, {0X80}},
	{0X07, 1, {0X05}},	//06
	{0X08, 1, {0X02}},	//01
	{0X09, 1, {0X80}},
	{0X0A, 1, {0X00}},
	{0X0B, 1, {0X00}},
	{0X0C, 1, {0X0A}},
	{0X0D, 1, {0X0A}},
	{0X0E, 1, {0X00}},
	{0X0F, 1, {0X00}},
	{0X10, 1, {0XE0}},	//F0
	{0X11, 1, {0XE4}},	//F4
	{0X12, 1, {0X04}},
	{0X13, 1, {0X00}},
	{0X14, 1, {0X00}},
	{0X15, 1, {0XC0}},
	{0X16, 1, {0X08}},
	{0X17, 1, {0X00}},
	{0X18, 1, {0X00}},
	{0X19, 1, {0X00}},
	{0X1A, 1, {0X00}},
	{0X1B, 1, {0X00}},
	{0X1C, 1, {0X00}},
	{0X1D, 1, {0X00}},
	{0X20, 1, {0X01}},
	{0X21, 1, {0X23}},
	{0X22, 1, {0X45}},
	{0X23, 1, {0X67}},
	{0X24, 1, {0X01}},
	{0X25, 1, {0X23}},
	{0X26, 1, {0X45}},
	{0X27, 1, {0X67}},
	{0X30, 1, {0X01}},
	{0X31, 1, {0X11}},
	{0X32, 1, {0X00}},
	{0X33, 1, {0XEE}},
	{0X34, 1, {0XFF}},
	{0X35, 1, {0XBB}},
	{0X36, 1, {0XCA}},
	{0X37, 1, {0XDD}},
	{0X38, 1, {0XAC}},
	{0X39, 1, {0X76}},
	{0X3A, 1, {0X67}},
	{0X3B, 1, {0X22}},
	{0X3C, 1, {0X22}},
	{0X3D, 1, {0X22}},
	{0X3E, 1, {0X22}},
	{0X3F, 1, {0X22}},
	{0X40, 1, {0X22}},
	{0X52, 1, {0X10}},
	{0X53, 1, {0X10}},
	//{0X54, 1, {0X13}},
	{0XFF, 5, {0XFF, 0X98, 0X06, 0X04, 0X07}},	//Change to page 0
	{0X17, 1, {0X22}},	//Display Access Control
	{0X02, 1, {0X77}},	//Write Display Brightness Value
	{0XE1, 1, {0X79}},	//Write CTRL Display Value
	{0X06, 1, {0X11}},
	{0X26, 1, {0XB2}},

	{0XFF, 5, {0XFF, 0X98, 0X06, 0X04, 0X00}},	//Change to page 0
	//{0X55, 1, {0XB0}},
	{0x11,01,{0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0x29,01,{0x00}},
	{REGFLAG_DELAY, 10, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}},
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
 

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
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
 


////update initial param for IC ili9881c_dsi_vdo 0.01
//
//static void set_enn_gpio_pin(int value)
//{
//	 mt_set_gpio_mode(GPIO_LCM_ENN, GPIO_LCM_RST_M_GPIO);
//	 mt_set_gpio_pull_enable(GPIO_LCM_ENN, GPIO_PULL_ENABLE);
//	 mt_set_gpio_dir(GPIO_LCM_ENN, GPIO_DIR_OUT);
//	if(value) {	
//		mt_set_gpio_out(GPIO_LCM_ENN, GPIO_OUT_ONE);
//	} else {
//		mt_set_gpio_out(GPIO_LCM_ENN, GPIO_OUT_ZERO);	
//	}
//}
//
//static void set_enp_gpio_pin(int value)
//{
//	 mt_set_gpio_mode(GPIO_LCM_ENP, GPIO_LCM_RST_M_GPIO);
//	 mt_set_gpio_pull_enable(GPIO_LCM_ENP, GPIO_PULL_ENABLE);
//	 mt_set_gpio_dir(GPIO_LCM_ENP, GPIO_DIR_OUT);
//
//	if(value) {
//		mt_set_gpio_out(GPIO_LCM_ENP, GPIO_OUT_ONE);
//	 
//	} else {
//		mt_set_gpio_out(GPIO_LCM_ENP, GPIO_OUT_ZERO);
//	}
//}
//// yewei.wt,add,20160813,ed


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


	params->dsi.vertical_sync_active = 2;
	params->dsi.vertical_backporch = 20;
	params->dsi.vertical_frontporch = 20;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 20;         //10
	params->dsi.horizontal_backporch = 100;    //50
	params->dsi.horizontal_frontporch = 100;   //100
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	/* Bit rate calculation */
	params->dsi.PLL_CLOCK = 225;   //265


	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
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

	id0 = buffer[0];	/* should be 0x00 */
	id1 = buffer[1];	/* should be 0xaa */
	id2 = buffer[2];	/* should be 0x55 */
#ifdef BUILD_LK
	printf("%s, id0 = 0x%08x\n", __func__, id0);	/* should be 0x00 */
	printf("%s, id1 = 0x%08x\n", __func__, id1);	/* should be 0xaa */
	printf("%s, id2 = 0x%08x\n", __func__, id2);	/* should be 0x55 */
#endif

	if((id0==0x00)&&(id1==0x80)&&(id2==0x00))
	return 1;
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

	data_array[0] = 0x00280500;	/* Display Off */
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10);
	data_array[0] = 0x00100500;	/* Sleep In */
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
	lcm_set_gpio_output(GPIO_LCD_RST, 0);
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

LCM_DRIVER ili9806e_fwvga_dsi_vdo_hlt_hc_lcm_drv = {

	.name = "ili9806e_fwvga_dsi_vdo_hlt_hc",
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
