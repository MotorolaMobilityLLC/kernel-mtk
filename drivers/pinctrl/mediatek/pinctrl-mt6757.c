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
#include "pinctrl-mtk-mt6757.h"




#if 0
/**
 * struct mtk_pin_ies_smt_set - For special pins' ies and smt setting.
 * @start: The start pin number of those special pins.
 * @end: The end pin number of those special pins.
 * @offset: The offset of special setting register.
 * @bit: The bit of special setting register.
 */
struct mtk_pin_ies_smt_set {
	unsigned int start;
	unsigned int end;
	unsigned int offset;
	unsigned char bit;
};

#define MTK_PIN_IES_SMT_SET(_start, _end, _offset, _bit)	\
	{	\
		.start = _start,	\
		.end = _end,	\
		.bit = _bit,	\
		.offset = _offset,	\
	}
#endif

#if 0
/**
 * struct mtk_pin_spec_pupd_set - For special pins' pull up/down setting.
 * @pin: The pin number.
 * @offset: The offset of special pull up/down setting register.
 * @pupd_bit: The pull up/down bit in this register.
 * @r0_bit: The r0 bit of pull resistor.
 * @r1_bit: The r1 bit of pull resistor.
 */
struct mtk_pin_spec_pupd_set {
	unsigned int pin;
	unsigned int offset;
	unsigned char pupd_bit;
	unsigned char r1_bit;
	unsigned char r0_bit;
};

#define MTK_PIN_PUPD_SPEC(_pin, _offset, _pupd, _r1, _r0)	\
	{	\
		.pin = _pin,	\
		.offset = _offset,	\
		.pupd_bit = _pupd,	\
		.r1_bit = _r1,		\
		.r0_bit = _r0,		\
	}
#endif

#if 0
static const struct mtk_pin_spec_pupd_set mt6735_spec_pupd[] = {

	MTK_PIN_PUPD_SPEC(45, 0x980, 2, 1, 0),
	MTK_PIN_PUPD_SPEC(46, 0x980, 6, 5, 4),
	MTK_PIN_PUPD_SPEC(81, 0xA80, 2, 1, 0),
	MTK_PIN_PUPD_SPEC(82, 0xA80, 6, 5, 4),
	MTK_PIN_PUPD_SPEC(83, 0xA80, 10, 9, 8),
	MTK_PIN_PUPD_SPEC(84, 0xA80, 18, 17, 16),
	MTK_PIN_PUPD_SPEC(85, 0xA80, 22, 21, 20),
	MTK_PIN_PUPD_SPEC(86, 0xA80, 26, 25, 24),
	MTK_PIN_PUPD_SPEC(160, 0xC90, 3, 2, 1),
	MTK_PIN_PUPD_SPEC(161, 0xC90, 7, 6, 5),
	MTK_PIN_PUPD_SPEC(162, 0xC90, 11, 10, 9),
	MTK_PIN_PUPD_SPEC(163, 0xC90, 19, 18, 17),
	MTK_PIN_PUPD_SPEC(164, 0xC90, 23, 22, 21),
	MTK_PIN_PUPD_SPEC(165, 0xC90, 27, 26, 25),
	MTK_PIN_PUPD_SPEC(166, 0xC80, 2, 1, 0),
	MTK_PIN_PUPD_SPEC(167, 0xC80, 6, 5, 4),
	MTK_PIN_PUPD_SPEC(168, 0xC80, 10, 9, 8),
	MTK_PIN_PUPD_SPEC(169, 0xC80, 14, 13, 12),
	MTK_PIN_PUPD_SPEC(170, 0xC80, 18, 17, 16),
	MTK_PIN_PUPD_SPEC(171, 0xC80, 22, 21, 20),
	MTK_PIN_PUPD_SPEC(172, 0xD80, 2, 1, 0),
	MTK_PIN_PUPD_SPEC(173, 0xD80, 10, 9, 8),
	MTK_PIN_PUPD_SPEC(174, 0xD80, 6, 5, 4),
	MTK_PIN_PUPD_SPEC(175, 0xD80, 14, 13, 12),
	MTK_PIN_PUPD_SPEC(176, 0xD80, 18, 17, 16),
	MTK_PIN_PUPD_SPEC(177, 0xD80, 22, 21, 20),
	MTK_PIN_PUPD_SPEC(178, 0xD80, 26, 25, 24),
	MTK_PIN_PUPD_SPEC(179, 0xD80, 30, 29, 28),
	MTK_PIN_PUPD_SPEC(180, 0xD90, 2, 1, 0),
	MTK_PIN_PUPD_SPEC(181, 0xD90, 6, 5, 4),
	MTK_PIN_PUPD_SPEC(182, 0xD90, 10, 9, 8),
	MTK_PIN_PUPD_SPEC(183, 0xD90, 14, 13, 12),
	MTK_PIN_PUPD_SPEC(198, 0x880, 2, 1, 0),
	MTK_PIN_PUPD_SPEC(199, 0x880, 6, 5, 4),
	MTK_PIN_PUPD_SPEC(200, 0x880, 10, 9, 8),
	MTK_PIN_PUPD_SPEC(201, 0x880, 14, 13, 12),
	MTK_PIN_PUPD_SPEC(202, 0x880, 18, 17, 16),
	MTK_PIN_PUPD_SPEC(203, 0x880, 22, 21, 20),
	MTK_PIN_PUPD_SPEC(45, 0x980, 2, 1, 0),
	MTK_PIN_PUPD_SPEC(46, 0x980, 6, 5, 4),
	MTK_PIN_PUPD_SPEC(81, 0xA80, 2, 1, 0),
	MTK_PIN_PUPD_SPEC(82, 0xA80, 6, 5, 4),
	MTK_PIN_PUPD_SPEC(83, 0xA80, 10, 9, 8),
	MTK_PIN_PUPD_SPEC(84, 0xA80, 18, 17, 16),
	MTK_PIN_PUPD_SPEC(85, 0xA80, 22, 21, 20),
	MTK_PIN_PUPD_SPEC(86, 0xA80, 26, 25, 24),
	/* MTK_PIN_PUPD_SPEC(160,0xC90,3,2,1), */
	/* MTK_PIN_PUPD_SPEC(161,0xC90,7,6,5), */
	/* MTK_PIN_PUPD_SPEC(162,0xC90,11,10,9), */
	/* MTK_PIN_PUPD_SPEC(163,0xC90,19,18,17), */
	/* MTK_PIN_PUPD_SPEC(164,0xC90,23,22,21), */
	/* MTK_PIN_PUPD_SPEC(165,0xC90,27,26,25), */
	MTK_PIN_PUPD_SPEC(166, 0xC80, 2, 1, 0),
	MTK_PIN_PUPD_SPEC(167, 0xC80, 6, 5, 4),
	MTK_PIN_PUPD_SPEC(168, 0xC80, 10, 9, 8),
	MTK_PIN_PUPD_SPEC(169, 0xC80, 14, 13, 12),
	MTK_PIN_PUPD_SPEC(170, 0xC80, 18, 17, 16),
	MTK_PIN_PUPD_SPEC(171, 0xC80, 22, 21, 20),
	MTK_PIN_PUPD_SPEC(172, 0xD80, 2, 1, 0),
	MTK_PIN_PUPD_SPEC(173, 0xD80, 10, 9, 8),
	MTK_PIN_PUPD_SPEC(174, 0xD80, 6, 5, 4),
	MTK_PIN_PUPD_SPEC(175, 0xD80, 14, 13, 12),
	MTK_PIN_PUPD_SPEC(176, 0xD80, 18, 17, 16),
	MTK_PIN_PUPD_SPEC(177, 0xD80, 22, 21, 20),
	MTK_PIN_PUPD_SPEC(178, 0xD80, 26, 25, 24),
	MTK_PIN_PUPD_SPEC(179, 0xD80, 30, 29, 28),
	MTK_PIN_PUPD_SPEC(180, 0xD90, 2, 1, 0),
	MTK_PIN_PUPD_SPEC(181, 0xD90, 6, 5, 4),
	MTK_PIN_PUPD_SPEC(182, 0xD90, 10, 9, 8),
	MTK_PIN_PUPD_SPEC(183, 0xD90, 14, 13, 12),
	MTK_PIN_PUPD_SPEC(198, 0x880, 2, 1, 0),
	MTK_PIN_PUPD_SPEC(199, 0x880, 6, 5, 4),
	MTK_PIN_PUPD_SPEC(200, 0x880, 10, 9, 8),
	MTK_PIN_PUPD_SPEC(201, 0x880, 14, 13, 12),
	MTK_PIN_PUPD_SPEC(202, 0x880, 18, 17, 16),
	MTK_PIN_PUPD_SPEC(203, 0x880, 22, 21, 20),
};
#endif

#if 0
static int spec_pull_set(struct regmap *regmap, unsigned int pin,
		unsigned char align, bool isup, unsigned int r1r0)
{
	unsigned int i;
	unsigned int reg_pupd, reg_set, reg_rst;
	unsigned int bit_pupd, bit_r0, bit_r1;
	const struct mtk_pin_spec_pupd_set *spec_pupd_pin;
	bool find = false;

	for (i = 0; i < ARRAY_SIZE(mt6735_spec_pupd); i++) {
		if (pin == mt6735_spec_pupd[i].pin) {
			find = true;
			break;
		}
	}

	if (!find)
		return -EINVAL;

	spec_pupd_pin = mt6735_spec_pupd + i;
	reg_set = spec_pupd_pin->offset + align;
	reg_rst = spec_pupd_pin->offset + (align << 1);

	if (isup)
		reg_pupd = reg_rst;
	else
		reg_pupd = reg_set;

	bit_pupd = BIT(spec_pupd_pin->pupd_bit);
	regmap_write(regmap, reg_pupd, bit_pupd);

	bit_r0 = BIT(spec_pupd_pin->r0_bit);
	bit_r1 = BIT(spec_pupd_pin->r1_bit);

	switch (r1r0) {
	case MTK_PUPD_SET_R1R0_00:
		regmap_write(regmap, reg_rst, bit_r0);
		regmap_write(regmap, reg_rst, bit_r1);
		break;
	case MTK_PUPD_SET_R1R0_01:
		regmap_write(regmap, reg_set, bit_r0);
		regmap_write(regmap, reg_rst, bit_r1);
		break;
	case MTK_PUPD_SET_R1R0_10:
		regmap_write(regmap, reg_rst, bit_r0);
		regmap_write(regmap, reg_set, bit_r1);
		break;
	case MTK_PUPD_SET_R1R0_11:
		regmap_write(regmap, reg_set, bit_r0);
		regmap_write(regmap, reg_set, bit_r1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
#endif

#if 0
static const struct mtk_pin_ies_smt_set mt6735_ies_smt_set[] = {
	MTK_PIN_IES_SMT_SET(0, 4, 0x930, 1),
	MTK_PIN_IES_SMT_SET(5, 9, 0x930, 2),
	MTK_PIN_IES_SMT_SET(10, 13, 0x930, 10),
	MTK_PIN_IES_SMT_SET(14, 15, 0x940, 10),
	MTK_PIN_IES_SMT_SET(16, 16, 0x930, 0),
	MTK_PIN_IES_SMT_SET(17, 17, 0x950, 2),
	MTK_PIN_IES_SMT_SET(18, 21, 0x940, 3),
	MTK_PIN_IES_SMT_SET(29, 32, 0x930, 3),
	MTK_PIN_IES_SMT_SET(33, 33, 0x930, 4),
	MTK_PIN_IES_SMT_SET(34, 36, 0x930, 5),
	MTK_PIN_IES_SMT_SET(37, 38, 0x930, 6),
	MTK_PIN_IES_SMT_SET(39, 39, 0x930, 7),
	MTK_PIN_IES_SMT_SET(40, 41, 0x930, 9),
	MTK_PIN_IES_SMT_SET(42, 42, 0x940, 0),
	MTK_PIN_IES_SMT_SET(43, 44, 0x930, 11),
	MTK_PIN_IES_SMT_SET(45, 46, 0x930, 12),
	MTK_PIN_IES_SMT_SET(57, 64, 0xc20, 13),
	MTK_PIN_IES_SMT_SET(65, 65, 0xc10, 13),
	MTK_PIN_IES_SMT_SET(66, 66, 0xc00, 13),
	MTK_PIN_IES_SMT_SET(67, 67, 0xd10, 13),
	MTK_PIN_IES_SMT_SET(68, 68, 0xd00, 13),
	MTK_PIN_IES_SMT_SET(69, 72, 0x940, 14),
	MTK_PIN_IES_SMT_SET(73, 76, 0xc60, 13),
	MTK_PIN_IES_SMT_SET(77, 77, 0xc40, 13),
	MTK_PIN_IES_SMT_SET(78, 78, 0xc50, 13),
	MTK_PIN_IES_SMT_SET(79, 82, 0x940, 15),
	MTK_PIN_IES_SMT_SET(83, 83, 0x950, 0),
	MTK_PIN_IES_SMT_SET(84, 85, 0x950, 1),
	MTK_PIN_IES_SMT_SET(86, 91, 0x950, 2),
	MTK_PIN_IES_SMT_SET(92, 92, 0x930, 13),
	MTK_PIN_IES_SMT_SET(93, 95, 0x930, 14),
	MTK_PIN_IES_SMT_SET(96, 99, 0x930, 15),
	MTK_PIN_IES_SMT_SET(100, 103, 0xca0, 13),
	MTK_PIN_IES_SMT_SET(104, 104, 0xc80, 13),
	MTK_PIN_IES_SMT_SET(105, 105, 0xc90, 13),
	MTK_PIN_IES_SMT_SET(106, 107, 0x940, 4),
	MTK_PIN_IES_SMT_SET(108, 112, 0x940, 1),
	MTK_PIN_IES_SMT_SET(113, 116, 0x940, 2),
	MTK_PIN_IES_SMT_SET(117, 118, 0x940, 5),
	MTK_PIN_IES_SMT_SET(119, 124, 0x940, 6),
	MTK_PIN_IES_SMT_SET(125, 126, 0x940, 7),
	MTK_PIN_IES_SMT_SET(127, 127, 0x940, 0),
	MTK_PIN_IES_SMT_SET(128, 128, 0x950, 8),
	MTK_PIN_IES_SMT_SET(129, 130, 0x950, 9),
	MTK_PIN_IES_SMT_SET(131, 132, 0x950, 8),
	MTK_PIN_IES_SMT_SET(133, 134, 0x910, 8)
};
#endif

#if 0
int spec_ies_smt_set(struct regmap *regmap, unsigned int pin,
		unsigned char align, int value)
{
	unsigned int i, reg_addr, bit;
	bool find = false;

	for (i = 0; i < ARRAY_SIZE(mt6735_ies_smt_set); i++) {
		if (pin >= mt6735_ies_smt_set[i].start &&
				pin <= mt6735_ies_smt_set[i].end) {
			find = true;
			break;
		}
	}

	if (!find)
		return -EINVAL;

	if (value)
		reg_addr = mt6735_ies_smt_set[i].offset + align;
	else
		reg_addr = mt6735_ies_smt_set[i].offset + (align << 1);

	bit = BIT(mt6735_ies_smt_set[i].bit);
	regmap_write(regmap, reg_addr, bit);
	return 0;
}
#endif

static const struct mtk_pinctrl_devdata mt6757_pinctrl_data = {
	.pins = mtk_pins_mt6757,
	.npins = ARRAY_SIZE(mtk_pins_mt6757),
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
	.type1_start = 204,
	.type1_end = 204,
	.port_shf = 4,
	.port_mask = 0xf,
	.port_align = 4,
	.chip_type = MTK_CHIP_TYPE_BASE,
	/*
	.eint_offsets = {
		.name = "mt6735_eint",
		.stat      = 0x000,
		.ack       = 0x040,
		.mask      = 0x080,
		.mask_set  = 0x0c0,
		.mask_clr  = 0x100,
		.sens      = 0x140,
		.sens_set  = 0x180,
		.sens_clr  = 0x1c0,
		.soft      = 0x200,
		.soft_set  = 0x240,
		.soft_clr  = 0x280,
		.pol       = 0x300,
		.pol_set   = 0x340,
		.pol_clr   = 0x380,
		.dom_en    = 0x400,
		.dbnc_ctrl = 0x500,
		.dbnc_set  = 0x600,
		.dbnc_clr  = 0x700,
		.port_mask = 7,
		.ports     = 6,
	},
	.ap_num = 224,
	.db_cnt = 16,
	*/
};

static int mt6757_pinctrl_probe(struct platform_device *pdev)
{
	pr_warn("mt6757 pinctrl probe\n");
	return mtk_pctrl_init(pdev, &mt6757_pinctrl_data, NULL);
}

static struct of_device_id mt6757_pctrl_match[] = {
	{
		.compatible = "mediatek,mt6757-pinctrl",
	}, {
	}
};
MODULE_DEVICE_TABLE(of, mt6757_pctrl_match);

static struct platform_driver mtk_pinctrl_driver = {
	.probe = mt6757_pinctrl_probe,
	.driver = {
		.name = "mediatek-mt6757-pinctrl",
		.owner = THIS_MODULE,
		.of_match_table = mt6757_pctrl_match,
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
