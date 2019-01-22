/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#else
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <asm-generic/gpio.h>

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#endif
#endif

#include "lcm_drv.h"

#define REGFLAG_DELAY       0xFE
#define REGFLAG_END_OF_TABLE    0xFF   /* END OF REGISTERS MARKER */

#define LCM_DSI_CMD_MODE    0

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/**
 * Local Variables
 */
static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

/**
 * Local Functions
 */
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)   lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)              lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)  lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg                lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)   lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
#ifndef ASSERT
#define ASSERT(expr)                    \
do {                        \
if (expr)               \
break;              \
pr_debug("DDP ASSERT FAILED %s, %d\n",  \
FILE__, __LINE__);     \
WARN_ON();                  \
} while (0)
#endif

struct LCM_setting_table {
	unsigned cmd;
	unsigned char count;
	unsigned char para_list[64];
};

#if 1
static void init_lcm_registers(void)
{
	unsigned int data_array[16];

#ifdef BUILD_LK
	printf("%s, LK\n", __func__);
#else
	pr_debug("%s, kernel", __func__);
#endif

	/* data_array[0] = 0x00110500;  //software reset */
	/* dsi_set_cmdq(data_array, 1, 1); */
	MDELAY(10);
	/* DSI_clk_HS_mode(1); */
	MDELAY(80);

	/* data_array[0] = 0x00290500;  //display on */
	/* dsi_set_cmdq(data_array, 1, 1); */

	data_array[0] = 0x00003200;  /* display on */
	dsi_set_cmdq(data_array, 1, 1);


}
#endif

#ifndef BUILD_LK
static struct regulator *lcm_vgp;

/* get LDO supply */
static int lcm_get_vgp_supply(struct device *dev)
{
	int ret;
	struct regulator *lcm_vgp_ldo;

	pr_debug("LCM: lcm_get_vgp_supply is going\n");

	lcm_vgp_ldo = devm_regulator_get(dev, "reg-lcm");
	if (IS_ERR(lcm_vgp_ldo)) {
		ret = PTR_ERR(lcm_vgp_ldo);
		dev_notice(dev, "failed to get reg-lcm LDO, %d\n", ret);
		return ret;
	}

	pr_debug("LCM: lcm get supply ok.\n");

	/* get current voltage settings */
	ret = regulator_get_voltage(lcm_vgp_ldo);
	pr_debug("lcm LDO voltage = %d in LK stage\n", ret);

	lcm_vgp = lcm_vgp_ldo;

	return ret;
}

int lcm_vgp_supply_enable(void)
{
	int ret;
	unsigned int volt;

	pr_debug("LCM: lcm_vgp_supply_enable\n");

	if (lcm_vgp == NULL)
		return 0;

	pr_debug("LCM: set regulator voltage lcm_vgp voltage to 1.8V\n");
	/* set voltage to 1.8V */
	ret = regulator_set_voltage(lcm_vgp, 1800000, 1800000);
	if (ret != 0) {
		pr_notice("LCM: lcm failed to set lcm_vgp voltage: %d\n", ret);
		return ret;
	}

	/* get voltage settings again */
	volt = regulator_get_voltage(lcm_vgp);
	if (volt == 1800000)
		pr_debug("LCM: check regulator voltage=1800000 pass!\n");
	else
		pr_debug("LCM: check regulator voltage=1800000 fail! (voltage: %d)\n", volt);

	ret = regulator_enable(lcm_vgp);
	if (ret != 0) {
		pr_notice("LCM: Failed to enable lcm_vgp: %d\n", ret);
		return ret;
	}

	return ret;
}

int lcm_vgp_supply_disable(void)
{
	int ret = 0;
	unsigned int isenable;

	if (lcm_vgp == NULL)
		return 0;

	/* disable regulator */
	isenable = regulator_is_enabled(lcm_vgp);

	pr_debug("LCM: lcm query regulator enable status[%d]\n", isenable);

	if (isenable) {
		ret = regulator_disable(lcm_vgp);
		if (ret != 0) {
			pr_notice("LCM: lcm failed to disable lcm_vgp: %d\n", ret);
			return ret;
		}
		/* verify */
		isenable = regulator_is_enabled(lcm_vgp);
		if (!isenable)
			pr_notice("LCM: lcm regulator disable pass\n");
	}

	return ret;
}

static int lcm_driver_probe(struct device *dev, void const *data)
{
	pr_debug("LCM: lcm_driver_probe\n");
	lcm_get_vgp_supply(dev);
	lcm_vgp_supply_enable();

	return 0;
}

static const struct of_device_id lcm_platform_of_match[] = {
	{
		.compatible = "nt,nt71397_wuxga_dsi_vdo_nt65902",
		.data = 0,
	}, {
		/* sentinel */
	}
};

MODULE_DEVICE_TABLE(of, platform_of_match);

static int lcm_platform_probe(struct platform_device *pdev)
{
	const struct of_device_id *id;

	id = of_match_node(lcm_platform_of_match, pdev->dev.of_node);
	if (!id)
		return -ENODEV;

	return lcm_driver_probe(&pdev->dev, id->data);
}

static struct platform_driver lcm_driver = {
	.probe = lcm_platform_probe,
	.driver = {
		.name = "nt71397_wuxga_dsi_vdo_nt65902",
		.owner = THIS_MODULE,
		.of_match_table = lcm_platform_of_match,
	},
};

static int __init lcm_init(void)
{
	if (platform_driver_register(&lcm_driver)) {
		pr_notice("LCM: failed to register this driver!\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit lcm_exit(void)
{
	platform_driver_unregister(&lcm_driver);
}

late_initcall(lcm_init);
module_exit(lcm_exit);
MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("LCM display subsystem driver");
MODULE_LICENSE("GPL");
#endif
/* --------------------------------------------------------------------------- */
/* Local Constants */
/* --------------------------------------------------------------------------- */
#define FRAME_WIDTH  (1920)
#define FRAME_HEIGHT (1200)

#define GPIO_OUT_ONE  1
#define GPIO_OUT_ZERO 0

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

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
#else
	params->dsi.mode = SYNC_EVENT_VDO_MODE;
#endif

	/* DSI */
	/* Command mode setting */
	/* Three lane or Four lane */
	params->dsi.LANE_NUM = LCM_FOUR_LANE;

	/* The following defined the format for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;

	/* Video mode setting */
	params->dsi.intermediat_buffer_num = 0;

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.word_count = FRAME_WIDTH * 3;

	params->dsi.vertical_sync_active                = 2;/* 10; */
	params->dsi.vertical_backporch                  = 15;/* 20; */
	params->dsi.vertical_frontporch                 = 60;/* 20; modified by mtk */
	params->dsi.vertical_active_line                = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active              = 2;/* 20; */
	params->dsi.horizontal_backporch                = 60;/* 80; */
	params->dsi.horizontal_frontporch               = 80;/* 80; */
	params->dsi.horizontal_active_pixel             = FRAME_WIDTH;

	params->dsi.ssc_disable = 1;
	params->dsi.PLL_CLOCK = 455;
	params->dsi.cont_clock = 1;

}


static void lcm_init_lcm(void)
{
	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(20);

	/* init_lcm_registers(); */
}


static void lcm_suspend(void)
{

	lcm_vgp_supply_disable();
	MDELAY(20);

}


static void lcm_resume(void)
{


	pr_debug("[Kernel/LCM] lcm_resume() enter\n");

	lcm_vgp_supply_enable();

	MDELAY(200);/* Must > 50ms */
	init_lcm_registers();
}


LCM_DRIVER nt71397_wuxga_dsi_vdo_nt65902_lcm_drv = {
	.name       = "nt71397_wuxga_dsi_vdo_nt65902",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init_lcm,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	/* .esd_check       = lcm_esd_check, */
	/* .esd_recover = lcm_esd_recover, */
#if (LCM_DSI_CMD_MODE)
	/*.set_backlight    = lcm_setbacklight,*/
	/* .set_pwm        = lcm_setpwm, */
	/* .get_pwm        = lcm_getpwm, */
	/*.update         = lcm_update, */
#endif
};
