#define LOG_TAG "LCM"

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
#include "disp_dts_gpio.h"
#endif
#ifndef MACH_FPGA
#include <lcm_pmic.h>
#endif

#include "mtkfb_params.h"

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_info("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif


static const unsigned int BL_MIN_LEVEL = 20;
static struct LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define MDELAY(n)		(lcm_util.mdelay(n))
#define UDELAY(n)		(lcm_util.udelay(n))

#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
		lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
	  lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
		lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
/* #include <linux/jiffies.h> */
/* #include <linux/delay.h> */
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#endif

#define LCM_DSI_CMD_MODE        0
#define FRAME_WIDTH             (720)
#define FRAME_HEIGHT            (1600)

#define LCM_PHYSICAL_WIDTH	(67930)
#define LCM_PHYSICAL_HEIGHT	(150960)

#define REGFLAG_DELAY		0xFFFC
#define REGFLAG_UDELAY	        0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD
#define REGFLAG_RESET_LOW	0xFFFE
#define REGFLAG_RESET_HIGH	0xFFFF

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

extern void lcm_set_bias_pin_disable(unsigned int delay);
extern void lcm_set_bias_pin_enable(unsigned int value,unsigned int delay);
extern void lcm_set_bias_init(unsigned int value,unsigned int delay);

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{REGFLAG_DELAY, 40, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 100, {} }

};

static struct LCM_setting_table init_setting[] = {
	{0xFF,1,{0x23}},
	{0xFB,1,{0x01}},
	{0x00,1,{0x60}},
	{0x07,1,{0x00}},
	{0x08,1,{0x01}},
	{0x09,1,{0xE4}},
	{0xFF,1,{0x10}},
        {REGFLAG_DELAY, 1, {}},
	{0xFB,1,{0x01}},
	{0xBA,1,{0x03}},
	{0x35,1,{0x00}},
	{0x55,1,{0x01}},
	{0x53,1,{0x24}},
	{REGFLAG_DELAY, 10, {}},
	{0x29,1,{0x00}},
	{REGFLAG_DELAY, 20, {}},
	{0x11,1,{0x00}},
	{REGFLAG_DELAY, 100, {}},
};

static struct LCM_setting_table bl_level[] = {
	{ 0xFF, 0x01, {0x10} },
	{ 0xFB, 0x01, {0x01} },
	{ 0x51, 0x02, {0x06, 0x66} },
	{ REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table lcm_cabc_setting_ui[] = {
	{0xFF, 1, {0x10} },
	{0xFB, 1, {0x01} },
	{0x55, 1, {0x01} },
};

static struct LCM_setting_table lcm_cabc_setting_mv[] = {
	{0xFF, 1, {0x10} },
	{0xFB, 1, {0x01} },
	{0x55, 1, {0x03} },
};

static struct LCM_setting_table lcm_cabc_setting_disable[] = {
	{0xFF, 1, {0x10} },
	{0xFB, 1, {0x01} },
	{0x55, 1, {0x00} },
};

struct LCM_cabc_table {
	int cmd_num;
	struct LCM_setting_table *cabc_cmds;
};

//Make sure the seq keep consitent with definition of cabc_mode, otherwise it need remap
static struct LCM_cabc_table lcm_cabc_settings[] = {
	{ARRAY_SIZE(lcm_cabc_setting_ui), lcm_cabc_setting_ui},
	{ARRAY_SIZE(lcm_cabc_setting_mv), lcm_cabc_setting_mv},
	{ARRAY_SIZE(lcm_cabc_setting_disable), lcm_cabc_setting_disable},
};

static struct LCM_setting_table lcm_hbm_setting[] = {
	{0x51, 2, {0x06, 0x66} },	//80% PWM
	{0x51, 2, {0x07, 0XFF} },	//100% PWM
};

static void push_table(void *cmdq, struct LCM_setting_table *table,
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
	LCM_LOGI("[LCM] lcm_dfps_int start\n");
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
	dfps_params[0].vertical_frontporch = 980;
	//dfps_params[0].vertical_frontporch_for_low_power = 2500;

	/*if need mipi hopping params add here*/
	//dfps_params[0].dynamic_switch_mipi = 0;
	//dfps_params[0].PLL_CLOCK_dyn = 550;
	//dfps_params[0].horizontal_frontporch_dyn = 288;
	//dfps_params[0].vertical_frontporch_dyn = 1291;
	//dfps_params[0].vertical_frontporch_for_low_power_dyn = 2500;

	/*DPFS_LEVEL1*/
	dfps_params[1].level = DFPS_LEVEL1;
	dfps_params[1].fps = 9000;/*real fps * 100, to support float*/
	dfps_params[1].vact_timing_fps = 9000;/*real vact timing fps * 100*/
	/*if mipi clock solution*/
	/*dfps_params[1].PLL_CLOCK = xx;*/
	/*dfps_params[1].data_rate = xx; */
	/*if HFP solution*/
	/*dfps_params[1].horizontal_frontporch = xx;*/
	dfps_params[1].vertical_frontporch = 10;
	//dfps_params[1].vertical_frontporch_for_low_power = 2500;

	/*if need mipi hopping params add here*/
	//dfps_params[1].dynamic_switch_mipi = 0;
	//dfps_params[1].PLL_CLOCK_dyn = 550;
	//dfps_params[1].horizontal_frontporch_dyn = 288;
	//dfps_params[1].vertical_frontporch_dyn = 8;
	//dfps_params[1].vertical_frontporch_for_low_power_dyn = 2500;

	dsi->dfps_num = 2;
		LCM_LOGI("[LCM] lcm_dfps_int endn");
}
#endif

static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->physical_width = LCM_PHYSICAL_WIDTH/1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT/1000;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
	//lcm_dsi_mode = CMD_MODE;
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
	lcm_dsi_mode = SYNC_PULSE_VDO_MODE;
#endif
	LCM_LOGI("lcm_get_params lcm_dsi_mode %d\n", lcm_dsi_mode);
	params->dsi.switch_mode_enable = 0;

	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	/* video mode timing */
	params->dsi.vertical_sync_active = 2;
	params->dsi.vertical_backporch = 326;
	params->dsi.vertical_frontporch = 980;
	//params->dsi.vertical_frontporch_for_low_power = 0;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 4;
	params->dsi.horizontal_backporch = 44;
	params->dsi.horizontal_frontporch = 44;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_range = 4;
	params->dsi.ssc_disable = 1;

#if (LCM_DSI_CMD_MODE)
	/* this value must be in MTK suggested table */
	params->dsi.PLL_CLOCK = 270;
#else
	/* this value must be in MTK suggested table */
	params->dsi.PLL_CLOCK = 462;
#endif

	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;

	/*if need mipi hopping params add here*/
	//params->dsi.dynamic_switch_mipi = 1;
	//params->dsi.PLL_CLOCK_dyn = 550;
	//params->dsi.horizontal_frontporch_dyn = 288;

#ifdef CONFIG_MTK_HIGH_FRAME_RATE
	/****DynFPS start****/
	lcm_dfps_int(&(params->dsi));
	/****DynFPS end****/
#endif
}

static void lcm_init_power(void)
{
	LCM_LOGI("[LCM] lcm_init_power\n");
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_PWR_EN_OUT1);
	MDELAY(1);
	lcm_set_bias_init(18,1);
	MDELAY(10);
}

static void lcm_suspend_power(void)
{
	LCM_LOGI("[LCM] lcm_suspend_power\n");
	lcm_set_bias_pin_disable(1);
	MDELAY(1);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_PWR_EN_OUT0);
	LCM_LOGI("[LCM] lcm suspend power down.\n");
}

static void lcm_resume_power(void)
{
	LCM_LOGI("[LCM] lcm_resume_power\n");
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_PWR_EN_OUT1);
	MDELAY(1);
	lcm_set_bias_pin_enable(18,1);
	MDELAY(10);
}

static void lcm_init(void)
{
	LCM_LOGI("[LCM] lcm_init\n");
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
	MDELAY(10);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT0);
	MDELAY(10);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
	MDELAY(10);
	push_table(NULL, init_setting,
		sizeof(init_setting)/sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	LCM_LOGI("[LCM] lcm_suspend\n");
	push_table(NULL, lcm_suspend_setting,
		sizeof(lcm_suspend_setting)/sizeof(struct LCM_setting_table),
		1);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
	MDELAY(10);
}

static void lcm_resume(void)
{
	LCM_LOGI("[LCM] lcm_resume\n");
	lcm_init();
}

static unsigned int lcm_compare_id(void)
{

	return 1;

}


/* return TRUE: need recovery */
/* return FALSE: No need recovery */
static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK
	char buffer[3];
	int array[4];

	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x0A, buffer, 1);

	if (buffer[0] != 0x9C) {
		LCM_LOGI("[LCM ERROR] [0x0A]=0x%02x\n", buffer[0]);
		return TRUE;
	}
	LCM_LOGI("[LCM NORMAL] [0x0A]=0x%02x\n", buffer[0]);
	return FALSE;
#else
	return FALSE;
#endif

}

static unsigned int lcm_ata_check(unsigned char *buffer)
{
#ifndef BUILD_LK
	unsigned int ret = 0;
	unsigned int x0 = FRAME_WIDTH / 4;
	unsigned int x1 = FRAME_WIDTH * 3 / 4;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);

	unsigned int data_array[3];
	unsigned char read_buf[4];

	LCM_LOGI("ATA check size = 0x%x,0x%x,0x%x,0x%x\n",
		 x0_MSB, x0_LSB, x1_MSB, x1_LSB);
	data_array[0] = 0x0005390A;	/* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	/* read id return two byte,version and id */
	data_array[0] = 0x00043700;
	dsi_set_cmdq(data_array, 1, 1);

	read_reg_v2(0x2A, read_buf, 4);

	if ((read_buf[0] == x0_MSB) && (read_buf[1] == x0_LSB)
	    && (read_buf[2] == x1_MSB) && (read_buf[3] == x1_LSB))
		ret = 1;
	else
		ret = 0;

	x0 = 0;
	x1 = FRAME_WIDTH - 1;

	x0_MSB = ((x0 >> 8) & 0xFF);
	x0_LSB = (x0 & 0xFF);
	x1_MSB = ((x1 >> 8) & 0xFF);
	x1_LSB = (x1 & 0xFF);

	data_array[0] = 0x0005390A;	/* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	return ret;
#else
	return 0;
#endif
}

static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{

	//for 11bit
	LCM_LOGI("%s,nt36525c backlight: level = %d\n", __func__, level);

	bl_level[2].para_list[0] = (level&0x700)>>8;
	bl_level[2].para_list[1] = (level&0xFF);

	push_table(handle, bl_level,
		   sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_set_cmdq(void *handle, unsigned int *lcm_cmd,
		unsigned int *lcm_count, unsigned int *lcm_value)
{
	pr_info("%s,boe_nt36525c cmd:%d, value = %d\n", __func__, *lcm_cmd, *lcm_value);

	switch(*lcm_cmd) {
		case PARAM_HBM:
			LCM_LOGI("%s nt36525c -1 : para_list[0]=0x%x,para_list[1]=0x%x\n",__func__,lcm_hbm_setting[*lcm_value].para_list[0],lcm_hbm_setting[*lcm_value].para_list[1]);
			push_table(handle, &lcm_hbm_setting[*lcm_value], 1, 1);
			break;
		case PARAM_CABC:
			if (*lcm_value >= CABC_MODE_NUM) {
				pr_info("%s: invalid CABC mode:%d out of CABC_MODE_NUM:", *lcm_value, CABC_MODE_NUM);
			}
			else {
				unsigned int cmd_num = lcm_cabc_settings[*lcm_value].cmd_num;
				pr_info("%s: handle PARAM_CABC, mode=%d, cmd_num=%d", __func__, *lcm_value, cmd_num);
				push_table(handle, lcm_cabc_settings[*lcm_value].cabc_cmds, cmd_num, 1);
			}
			break;
		default:
			pr_err("%s,boe_nt36525c cmd:%d, unsupport\n", __func__, *lcm_cmd);
			break;
	}

	pr_info("%s,boe_nt36525c cmd:%d, value = %d done\n", __func__, *lcm_cmd, *lcm_value);
}

#ifdef CONFIG_LCM_NOTIFIY_SUPPORT
static bool lcm_set_recovery_notify(void)
{
	//return TRUE if TP need lcm notify
	//NVT touch recover need enable lcm notify
	return TRUE;
}
#endif

struct LCM_DRIVER mipi_mot_vid_boe_nt36525c_652_hdp_lcm_drv = {
	.name = "mipi_mot_vid_boe_nt36525c_652_hdp",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id = lcm_compare_id,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.esd_check = lcm_esd_check,
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
	.ata_check = lcm_ata_check,
	.set_lcm_cmd = lcm_set_cmdq,
#ifdef CONFIG_LCM_NOTIFIY_SUPPORT
	.set_lcm_notify = lcm_set_recovery_notify,
#endif
};
