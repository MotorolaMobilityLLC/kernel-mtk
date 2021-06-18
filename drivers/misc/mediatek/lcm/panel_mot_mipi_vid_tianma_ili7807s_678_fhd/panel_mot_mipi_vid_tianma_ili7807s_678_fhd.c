#ifdef BUILD_LK
#include <upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#else
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#endif

#include <linux/i2c-dev.h>
#include "lcm_i2c.h"

#include "mtkfb_params.h"

static struct LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v) (lcm_util.set_reset_pin(v))
#define MDELAY(n) (lcm_util.mdelay(n))

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

#define FRAME_WIDTH     (1080)
#define FRAME_HEIGHT    (2460)

#define REGFLAG_DELAY           0xFFFC
#define REGFLAG_END_OF_TABLE    0xFFFD

/* i2c control start */
#define LCM_I2C_ID_NAME "i2c_lcd_bias"

static int _lcm_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id);
static int _lcm_i2c_remove(struct i2c_client *client);

static const struct of_device_id _lcm_i2c_of_match[] = {
	{.compatible = "mediatek,i2c_lcd_bias",},
	{},
};

static const struct i2c_device_id _lcm_i2c_id[] = {
	{LCM_I2C_ID_NAME,0},
	{},
};

static struct i2c_driver _lcm_i2c_driver = {
	.id_table = _lcm_i2c_id,
	.probe = _lcm_i2c_probe,
	.remove = _lcm_i2c_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = LCM_I2C_ID_NAME,
		.of_match_table = _lcm_i2c_of_match,
	},
};

static int _lcm_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	pr_err("[LCM][I2C]%s", __func__);
	_lcm_i2c_client = client;
	return 0;
}

static int _lcm_i2c_remove(struct i2c_client *client)
{
	pr_err("[LCM][I2C]%s", __func__);
	_lcm_i2c_client = NULL;
	i2c_unregister_device(client);
	return 0;
}

static int _lcm_i2c_write_bytes(unsigned char addr, unsigned char value)
{
	int ret = 0;
	struct i2c_client *client = _lcm_i2c_client;
	char write_data[2] = {0};

	if(client == NULL)
	{
		pr_debug("[LCM][I2C] i2c_client is null\n");
		return 0;
	}
	write_data[0] = addr;
	write_data[1] = value;
	ret = i2c_master_send(client, write_data, 2);
	if(ret < 0)
		pr_debug("[LCM][I2C] i2c_write data fail!\n");
	return ret;
}

static int __init _lcm_i2c_init(void)
{
	pr_err("[LCM][I2C]%s\n", __func__);
	i2c_add_driver(&_lcm_i2c_driver);
	pr_err("[LCM][I2C]%s success\n", __func__);
	return 0;
}

static void __exit _lcm_i2c_exit(void)
{
	pr_debug("[LCM][I2C]%s\n", __func__);
	i2c_del_driver(&_lcm_i2c_driver);
}

module_init(_lcm_i2c_init);
module_exit(_lcm_i2c_exit);
/* i2c control end */

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{REGFLAG_DELAY, 20, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 120, {} },
};

static struct LCM_setting_table init_setting_cmd[] = {
	{0xFF, 3, {0x78, 0x07, 0x06} }, //120HZ
	{0x5F, 1, {0x24} },
	{0x4E, 1, {0xF0} },
	{0x4D, 1, {0x00} },
	{0x48, 1, {0x09} },
	{0xC7, 1, {0x05} },

	{0xFF, 3, {0x78, 0x07, 0x00} },	//Page0  120dsc
	{0x11, 1, {0x00} },
	{REGFLAG_DELAY, 60, {} },
	{0x29, 1, {0x00} },
	{REGFLAG_DELAY, 20, {} },

	{0xFF, 3, {0x78, 0x07, 0x03} },	//pwm 12bit, 28.32k
	{0x83, 1, {0x20} },
	{0x84, 1, {0x00} },

	{0xFF, 3, {0x78, 0x07, 0x06} },
	{0x08, 1, {0x20} },

	{0xFF, 3, {0x78, 0x07, 0x00} },	//Page0
	{0x51, 2, {0x0c, 0xcc}},
	{0x53, 1, {0x2c} },
	{0x55, 1, {0x01} },	//CABC:UI
	{0x35, 1, {0x00} },

	{0xFF, 3, {0x78, 0x07, 0x06} },
	{0x3E, 1, {0xE3} },
	{0x80, 1, {0x00} },
	{0xFF, 3, {0x78, 0x07, 0x08} },
	{0xFD, 2, {0x00, 0x9F} },
	{0xE1, 1, {0xD9} },
	{0xFD, 2, {0x00, 0x00} },
	{0xFF, 3, {0x78, 0x07, 0x00} },
};

static struct LCM_setting_table lcm_cabc_setting[] = {
	{0x55, 1, {0x01} },	//UI
	{0x55, 1, {0x03} },	//MV
	{0x55, 1, {0x00} },	//DISABLE
};

static struct LCM_setting_table lcm_hbm_setting[] = {
	{0x51, 2, {0x0C, 0XCC} },	//80% PWM
	{0x51, 2, {0x0F, 0XFF} },	//100% PWM
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
				MDELAY(table[i].count);
		case REGFLAG_END_OF_TABLE:
			break;
		default:
			dsi_set_cmdq_V22(cmdq, cmd,
				table[i].count,
				table[i].para_list,
				force_update);
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
	dsi->dfps_default_fps = 12000;/*real fps * 100, to support float*/
	dsi->dfps_def_vact_tim_fps = 12000;/*real vact timing fps * 100*/
	/* traversing array must less than DFPS_LEVELS */
	/* DPFS_LEVEL0 */
	dfps_params[0].level = DFPS_LEVEL0;
	dfps_params[0].fps = 6000;/*real fps * 100, to support float*/
	dfps_params[0].vact_timing_fps = 12000;/*real vact timing fps * 100*/
	/* if mipi clock solution */
	//dfps_params[0].PLL_CLOCK = 510;
	/* dfps_params[0].data_rate = xx; */
	/* if vfp solution */
	dfps_params[0].vertical_frontporch = 2580;
	//dfps_params[0].vertical_frontporch_for_low_power = 3550; //50hz

	/* DPFS_LEVEL1 */
	dfps_params[1].level = DFPS_LEVEL1;
	dfps_params[1].fps = 12000;/*real fps * 100, to support float*/
	dfps_params[1].vact_timing_fps = 12000;/*real vact timing fps * 100*/
	/* if mipi clock solution */
	/*dfps_params[1].PLL_CLOCK = 510;*/
	/* dfps_params[1].data_rate = xx; */
	/* if vfp solution */
	dfps_params[1].vertical_frontporch = 50;
	//dfps_params[1].vertical_frontporch_for_low_power = 880; //90hz
	dsi->dfps_num = 2;
}
#endif

static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	params->type = LCM_TYPE_DSI;
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->dsi.mode = SYNC_PULSE_VDO_MODE;

	params->dsi.LANE_NUM = LCM_FOUR_LANE;

	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = 2;
	params->dsi.vertical_backporch = 20;
	params->dsi.vertical_frontporch = 2580;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

 	params->dsi.horizontal_sync_active = 4;
	params->dsi.horizontal_backporch = 59;
	params->dsi.horizontal_frontporch = 102;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	params->dsi.PLL_CLOCK = 480;
	params->physical_height = 161;
	params->physical_width = 70;

	params->dsi.dsc_enable = 0;
	params->dsi.ssc_disable = 1;
	params->dsi.bdg_ssc_disable = 1;

	params->dsi.dsc_params.ver = 17;
	params->dsi.dsc_params.slice_mode = 1;
	params->dsi.dsc_params.rgb_swap = 0;
	params->dsi.dsc_params.dsc_cfg = 34;
	params->dsi.dsc_params.rct_on = 1;
	params->dsi.dsc_params.bit_per_channel = 8;
	params->dsi.dsc_params.dsc_line_buf_depth = 9;
	params->dsi.dsc_params.bp_enable = 1;
	params->dsi.dsc_params.bit_per_pixel = 128;
	params->dsi.dsc_params.pic_height = 2460;
	params->dsi.dsc_params.pic_width = 1080;
	params->dsi.dsc_params.slice_height = 10;
	params->dsi.dsc_params.slice_width = 540;
	params->dsi.dsc_params.chunk_size = 540;
	params->dsi.dsc_params.xmit_delay = 512;
	params->dsi.dsc_params.dec_delay = 526;
	params->dsi.dsc_params.scale_value = 32;
	params->dsi.dsc_params.increment_interval = 237;
	params->dsi.dsc_params.decrement_interval = 7;
	params->dsi.dsc_params.line_bpg_offset = 12;
	params->dsi.dsc_params.nfl_bpg_offset = 2731;
	params->dsi.dsc_params.slice_bpg_offset = 2604;
	params->dsi.dsc_params.initial_offset = 6144;
	params->dsi.dsc_params.final_offset = 4366;
	params->dsi.dsc_params.flatness_minqp = 3;
	params->dsi.dsc_params.flatness_maxqp = 12;
	params->dsi.dsc_params.rc_model_size = 8192;
	params->dsi.dsc_params.rc_edge_factor = 6;
	params->dsi.dsc_params.rc_quant_incr_limit0 = 11;
	params->dsi.dsc_params.rc_quant_incr_limit1 = 11;
	params->dsi.dsc_params.rc_tgt_offset_hi = 3;
	params->dsi.dsc_params.rc_tgt_offset_lo = 3;

	params->dsi.bdg_dsc_enable = 1;

	params->density = 480;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0a;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;
        params->dsi.lcm_esd_check_table[1].cmd = 0x0b;
        params->dsi.lcm_esd_check_table[1].count = 1;
        params->dsi.lcm_esd_check_table[1].para_list[0] = 0x00;

	#ifdef CONFIG_MTK_HIGH_FRAME_RATE
	/****DynFPS start****/
	lcm_dfps_int(&(params->dsi));
	/****DynFPS end****/
	#endif
}

static void lcm_init_power(void)
{
	lcm_util.set_lcd_ldo_en(1);
	if(lcm_util.set_gpio_lcd_enp_bias) {
		lcm_util.set_gpio_lcd_enp_bias(1);
		_lcm_i2c_write_bytes(0x00, 0x12);
		_lcm_i2c_write_bytes(0x01, 0x12);
	}
	else
		pr_debug("[LCM]set_gpio_lcd_enp_bias not defined\n");
}

static void lcm_suspend_power(void)
{
	pr_info("[lcm]lcm_suspend_power");
	//SET_RESET_PIN(0);
	if(lcm_util.set_gpio_lcd_enp_bias) {
		lcm_util.set_gpio_lcd_enp_bias(0);
	}
}

static void lcm_resume_power(void)
{
	//SET_RESET_PIN(0);
	lcm_init_power();
}

static void lcm_init(void)
{
	pr_info("[LCM]lcm_init\n");

	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(20);
	push_table(NULL, init_setting_cmd,
			ARRAY_SIZE(init_setting_cmd), 1);

}

static void lcm_suspend(void)
{
	pr_info("[LCM]lcm_suspend\n");

	push_table(NULL, lcm_suspend_setting,
			ARRAY_SIZE(lcm_suspend_setting), 1);
	MDELAY(10);
}

static void lcm_resume(void)
{
	lcm_init();
}


static void lcm_setbacklight(/*void *handle, */unsigned int level)
{
	pr_info("lcm_setbacklight.");

	return;
}

static void lcm_set_cmdq(void *handle, unsigned int *lcm_cmd,
		unsigned int *lcm_count, unsigned int *lcm_value)
{
	pr_info("%s,ili7807s cmd:%d, value = %d\n", __func__, *lcm_cmd, *lcm_value);

	switch(*lcm_cmd) {
		case PARAM_HBM:
			push_table(handle, &lcm_hbm_setting[*lcm_value], 1, 1);
			break;
		case PARAM_CABC:
			push_table(handle, &lcm_cabc_setting[*lcm_value], 1, 1);
			break;
		default:
			pr_err("%s,ili7807s cmd:%d, unsupport\n", __func__, *lcm_cmd);
			break;
	}

	pr_info("%s,ili7807s cmd:%d, value = %d done\n", __func__, *lcm_cmd, *lcm_value);

}

struct LCM_DRIVER mipi_mot_vid_tianma_ili7807s_fhd_678_lcm_drv = {
	.name = "mipi_mot_vid_tianma_ili7807s_fhd_678",
	.supplier = "tianma",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.set_backlight = lcm_setbacklight,
//	.set_backlight_cmdq = lcm_setbacklight_cmdq,
        .set_lcm_cmd = lcm_set_cmdq,
        .tp_gesture_status = GESTURE_OFF,
};
