/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Hongzhou.Yang <hongzhou.yang@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/regmap.h>
#include <dt-bindings/pinctrl/mt65xx.h>
#include "pinctrl-mtk-common.h"
#include "pinctrl-mtk-mt6799.h"

static const struct mtk_pinctrl_devdata mt6799_pinctrl_data = {
	.pins = mtk_pins_mt6799,
	.npins = ARRAY_SIZE(mtk_pins_mt6799),
	.grp_desc = NULL,
	.n_grp_cls = 0,
	.pin_drv_grp = NULL,
	.n_pin_drv_grps = 0,
	/* .spec_pull_set = spec_pull_set, */
	/* .spec_ies_smt_set = spec_ies_smt_set, */
	.spec_set_gpio_mode = mt_set_gpio_mode,
	.mt_set_gpio_dir = mt_set_gpio_dir,
	.mt_get_gpio_dir = mt_get_gpio_dir,
	.mt_get_gpio_out = mt_get_gpio_out,
	.mt_set_gpio_out = mt_set_gpio_out,
	.mt_set_gpio_ies = mt_set_gpio_ies,
	.mt_set_gpio_smt = mt_set_gpio_smt,
	.mt_set_gpio_pull_enable =  mt_set_gpio_pull_enable,
	.mt_set_gpio_pull_select = mt_set_gpio_pull_select,
	.mt_get_gpio_in = mt_get_gpio_in,
	/* .dir_offset = 0x0000, */
	/* .pullen_offset = 0x0100, */
	/* .pullsel_offset = 0x0200, */
	/* .dout_offset = 0x0100, */
	/* .din_offset = 0x0200, */
	/* .pinmux_offset = 0x300, */
	.type1_start = 232,
	.type1_end = 232,
	.port_shf = 4,
	.port_mask = 0xf,
	.port_align = 4,
};

static int mt6799_pinctrl_probe(struct platform_device *pdev)
{
	pr_warn("mt6799 pinctrl probe\n");
	return mtk_pctrl_init(pdev, &mt6799_pinctrl_data, NULL);
}

static const struct of_device_id mt6799_pctrl_match[] = {
	{
		.compatible = "mediatek,mt6799-pinctrl",
	}, {
	}
};
MODULE_DEVICE_TABLE(of, mt6799_pctrl_match);

static struct platform_driver mtk_pinctrl_driver = {
	.probe = mt6799_pinctrl_probe,
	.driver = {
		.name = "mediatek-mt6799-pinctrl",
		.owner = THIS_MODULE,
		.of_match_table = mt6799_pctrl_match,
	},
};

static int __init mtk_pinctrl_init(void)
{
	return platform_driver_register(&mtk_pinctrl_driver);
}

/* module_init(mtk_pinctrl_init); */

postcore_initcall(mtk_pinctrl_init);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek Pinctrl Driver");
MODULE_AUTHOR("Hongzhou Yang <hongzhou.yang@mediatek.com>");
