/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */


#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/regmap.h>
#include <dt-bindings/pinctrl/mt65xx.h>
#include "pinctrl-mtk-common.h"
#include "pinctrl-mtk-mt6763.h"
#include <mt-plat/mtk_gpio.h>

/* For type discreate/continuous PUPD + R1 + R0 */
static int mtk_pinctrl_set_gpio_pupd_r1r0(struct mtk_pinctrl *pctl, int pin,
		bool enable, bool isup, unsigned int r1r0)
{
	unsigned int pupd_r1r0 = 0, r0, r1, ret;

	/* For type continuous PUPD + R1 + R0 */
	if (r1r0 == MTK_PUPD_SET_R1R0_01)
		pupd_r1r0 = 1;
	else if (r1r0 == MTK_PUPD_SET_R1R0_10)
		pupd_r1r0 = 2;
	else if (r1r0 == MTK_PUPD_SET_R1R0_11)
		pupd_r1r0 = 3;
	else
		pupd_r1r0 = 0;
	if (!isup)
		pupd_r1r0 |= 0x4;

	ret = mtk_pinctrl_update_gpio_value(pctl, pin, pupd_r1r0,
		pctl->devdata->n_pin_pupd_r1r0, pctl->devdata->pin_pupd_r1r0_grps);
	if (ret == 0)
		return ret;

	if (!pctl->devdata->n_pin_pupd)
		return -EPERM;

	/* For type discreate PUPD + R1 + R0 */
	ret = mtk_pinctrl_update_gpio_value(pctl, pin, !isup,
		pctl->devdata->n_pin_pupd, pctl->devdata->pin_pupd_grps);
	if (ret == 0) {
		r0 = r1r0 & 0x1;
		r1 = (r1r0 & 0x2) >> 1;
		mtk_pinctrl_update_gpio_value(pctl, pin, r0,
			pctl->devdata->n_pin_r0, pctl->devdata->pin_r0_grps);
		mtk_pinctrl_update_gpio_value(pctl, pin, r1,
			pctl->devdata->n_pin_r1, pctl->devdata->pin_r1_grps);
		ret = 0;
	}
	return ret;
}

/* For type discreate/continuous PUPD + R1 + R0 */
static int mtk_pinctrl_get_gpio_pupd_r1r0(struct mtk_pinctrl *pctl, int pin)
{
	int ret;
	int bit_pupd, bit_r0, bit_r1;

	/* For type continuous PUPD + R1 + R0 */
	ret = mtk_pinctrl_get_gpio_value(pctl, pin,
		pctl->devdata->n_pin_pupd_r1r0, pctl->devdata->pin_pupd_r1r0_grps);
	if (ret >= 0) {
		bit_pupd = (ret & 0x4) ? 1 : 0; /* set for PD, clr for PU */
		bit_r0   = (ret & 0x1) ? MTK_PUPD_R1R0_BIT_R0 : 0;
		bit_r1   = (ret & 0x2) ? MTK_PUPD_R1R0_BIT_R1 : 0;
		return MTK_PUPD_R1R0_BIT_SUPPORT | bit_r1 | bit_r0 | bit_pupd;
	}

	if (!pctl->devdata->n_pin_pupd)
		return -EPERM;

	/* For type discreate PUPD + R1 + R0 */
	bit_pupd = mtk_pinctrl_get_gpio_value(pctl, pin,
		pctl->devdata->n_pin_pupd, pctl->devdata->pin_pupd_grps);
	if (bit_pupd != -EPERM) {
		bit_r1 = mtk_pinctrl_get_gpio_value(pctl, pin,
			pctl->devdata->n_pin_r1, pctl->devdata->pin_r1_grps) ? MTK_PUPD_R1R0_BIT_R1 : 0;
		bit_r0 = mtk_pinctrl_get_gpio_value(pctl, pin,
			pctl->devdata->n_pin_r0, pctl->devdata->pin_r0_grps) ? MTK_PUPD_R1R0_BIT_R0 : 0;
		return MTK_PUPD_R1R0_BIT_SUPPORT | bit_r1 | bit_r0 | bit_pupd;
	}
	return -EPERM;
}

/* For type PULLSEL + PULLEN */
static int mtk_pinctrl_set_gpio_pullsel_pullen(struct mtk_pinctrl *pctl, int pin,
		bool enable, bool isup, unsigned int r1r0)
{
	mtk_pinctrl_update_gpio_value(pctl, pin, isup,
		pctl->devdata->n_pin_pullsel, pctl->devdata->pin_pullsel_grps);
	mtk_pinctrl_update_gpio_value(pctl, pin, enable,
			pctl->devdata->n_pin_pullen, pctl->devdata->pin_pullen_grps);
	return 0;
}

/* For type PULLSEL + PULLEN */
static int mtk_pinctrl_get_gpio_pullsel_pullen(struct mtk_pinctrl *pctl, int pin)
{
	unsigned int pullsel = 0, pullen = 0;

	pullsel = mtk_pinctrl_get_gpio_value(pctl, pin,
		pctl->devdata->n_pin_pullsel, pctl->devdata->pin_pullsel_grps);
	pullen = mtk_pinctrl_get_gpio_value(pctl, pin,
		pctl->devdata->n_pin_pullen, pctl->devdata->pin_pullen_grps);
	if (pullsel == -EPERM || pullen == -EPERM)
		return -EPERM;
	if (pullen == 0)
		return 0;
	else if ((pullsel == 1) && (pullen == 1))
		return MTK_PUPD_BIT_PU;
	else if ((pullsel == 0) && (pullen == 1))
		return MTK_PUPD_BIT_PD;
	else
		return -EINVAL;
}

/* Not specifically for gettting pullsel of PULLSEL type,
 * For getting pull-up/pull-down/no-pull + pull-enable/pull-disable
 */
static int mtk_pinctrl_get_gpio_pullsel(struct mtk_pinctrl *pctl, int pin)
{
	int pull_val = 0;

	pull_val = mtk_pinctrl_get_gpio_pupd_r1r0(pctl, pin);
	if (pull_val == -EPERM) {
		pull_val = mtk_pinctrl_get_gpio_pullsel_pullen(pctl, pin);
		/*pull_val = [pu,pd], 10 is pull up, 01 is pull down*/
		if (pull_val == MTK_PUPD_BIT_PU)
			pull_val = GPIO_PULL_UP;
		else if (pull_val == MTK_PUPD_BIT_PD)
			pull_val = GPIO_PULL_DOWN;
		else if (pull_val == -EPERM)
			pull_val = GPIO_PULL_UNSUPPORTED;
		else
			pull_val = GPIO_NO_PULL;
	}
	return pull_val;
}

/* Specifically for gettting pullen of PULLSEL type or pull-resistors of PUPD+R0+R1 */
static int mtk_pinctrl_get_gpio_pullen(struct mtk_pinctrl *pctl, int pin)
{
	int pull_val = 0, pull_en;

	pull_val = mtk_pinctrl_get_gpio_pupd_r1r0(pctl, pin);
	if (pull_val == -EPERM) {
		pull_en = mtk_pinctrl_get_gpio_pullsel_pullen(pctl, pin);
		/*pull_en = [pu,pd], 10,01 pull enabel, others pull disable*/
		if ((pull_en == MTK_PUPD_BIT_PD) || (pull_en == MTK_PUPD_BIT_PU))
			pull_en = GPIO_PULL_ENABLE;
		else if (pull_en == 0)
			pull_en = GPIO_PULL_DISABLE;
		else
			pull_en = GPIO_PULL_UNSUPPORTED;
	} else if (pull_val & MTK_PUPD_R1R0_BIT_SUPPORT) {
		/*pull_val = [r1,r0,pupd], pull disabel 000,001, others enable*/
		if (MTK_PUPD_R1R0_GET_PULLEN(pull_val))
			pull_en = GPIO_PULL_ENABLE;
		else
			pull_en = GPIO_PULL_DISABLE;
	} else {
		return -EINVAL;
	}
	return pull_en;

}

static int mtk_pinctrl_set_gpio_pull(struct mtk_pinctrl *pctl,
		int pin, bool enable, bool isup, unsigned int arg)
{
	int ret;

/* #define GPIO_DEBUG */
#ifdef GPIO_DEBUG
	int pull_val;

	pr_warn("mtk_pinctrl_set_gpio_pull, pin = %d, enab = %d, sel = %d, arg = %u\n",
		pin, enable, isup, arg);
#endif
	ret = mtk_pinctrl_set_gpio_pupd_r1r0(pctl, pin, enable, !isup, arg);
	if (ret == 0) {
#ifdef GPIO_DEBUG
		pull_val = mtk_pinctrl_get_gpio_pullsel(pctl, pin);
		pr_warn("mtk_pinctrl_get_gpio_pull, pin = %d, enab = %d, sel = %d\n",
			pin,
			((pull_val >= 0) ? MTK_PUPD_R1R0_GET_PULLEN(pull_val) : -1),
			((pull_val >= 0) ? MTK_PUPD_R1R0_GET_PUPD(pull_val) : -1));
#endif
		goto out;
	}

	if (!pctl->devdata->pin_pullsel_grps) {
		ret = -EPERM;
		goto out;
	}

	ret = mtk_pinctrl_set_gpio_pullsel_pullen(pctl, pin, enable, isup, arg);
#ifdef GPIO_DEBUG
	if (ret == 0) {
		int enab = -1, sel = -1;

		pull_val = mtk_pinctrl_get_gpio_pullsel(pctl, pin);
		if (pull_val == GPIO_PULL_UP) {
			enab = 1;
			sel = 1;
		} else if (pull_val == GPIO_PULL_DOWN) {
			enab = 1;
			sel = 0;
		} else if (pull_val == GPIO_NO_PULL) {
			enab = 0;
			sel = 0;
		} else if (pull_val == GPIO_PULL_UNSUPPORTED) {
			enab = -1;
			sel = -1;
		}
		pr_warn("mtk_pinctrl_get_gpio_pull, pin = %d, enab = %d, sel = %d\n",
			pin, enab, sel);
	}
#endif

out:

	return ret;
}

int mtk_pinctrl_get_gpio_mode_for_eint(int pin)
{
	return mtk_pinctrl_get_gpio_value(pctl, pin,
		pctl->devdata->n_pin_mode, pctl->devdata->pin_mode_grps);
}

static const struct mtk_pinctrl_devdata mt6763_pinctrl_data = {
	.pins = mtk_pins_mt6763,
	.npins = ARRAY_SIZE(mtk_pins_mt6763),
	.pin_mode_grps = mtk_pin_info_mode,
	.n_pin_mode = ARRAY_SIZE(mtk_pin_info_mode),
	.pin_drv_grps = mtk_pin_info_drv,
	.n_pin_drv = ARRAY_SIZE(mtk_pin_info_mode),
	.pin_smt_grps = mtk_pin_info_smt,
	.n_pin_smt = ARRAY_SIZE(mtk_pin_info_smt),
	.pin_ies_grps = mtk_pin_info_ies,
	.n_pin_ies = ARRAY_SIZE(mtk_pin_info_ies),
	.pin_pullsel_grps = mtk_pin_info_pullsel,
	.n_pin_pullsel = ARRAY_SIZE(mtk_pin_info_pullsel),
	.pin_pullen_grps = mtk_pin_info_pullen,
	.n_pin_pullen = ARRAY_SIZE(mtk_pin_info_pullen),
	.pin_pupd_r1r0_grps = mtk_pin_info_pupd_r1r0,
	.n_pin_pupd_r1r0 = ARRAY_SIZE(mtk_pin_info_pupd_r1r0),
	.pin_dout_grps = mtk_pin_info_dataout,
	.n_pin_dout = ARRAY_SIZE(mtk_pin_info_dataout),
	.pin_din_grps = mtk_pin_info_datain,
	.n_pin_din = ARRAY_SIZE(mtk_pin_info_datain),
	.pin_dir_grps = mtk_pin_info_dir,
	.n_pin_dir = ARRAY_SIZE(mtk_pin_info_dir),
	.mtk_pctl_set_pull = mtk_pinctrl_set_gpio_pull,
	.mtk_pctl_get_pull_sel = mtk_pinctrl_get_gpio_pullsel,
	.mtk_pctl_get_pull_en = mtk_pinctrl_get_gpio_pullen,
	.type1_start = 151,
	.type1_end = 151,
	.regmap_num = 9,
	.port_shf = 4,
	.port_mask = 0xf,
	.port_align = 4,
};

static int mt6763_pinctrl_probe(struct platform_device *pdev)
{
	pr_warn("mt6763 pinctrl probe\n");
	return mtk_pctrl_init(pdev, &mt6763_pinctrl_data, NULL);
}

static const struct of_device_id mt6763_pctrl_match[] = {
	{
		.compatible = "mediatek,mt6763-pinctrl",
	}, {
	}
};
MODULE_DEVICE_TABLE(of, mt6763_pctrl_match);

static struct platform_driver mtk_pinctrl_driver = {
	.probe = mt6763_pinctrl_probe,
	.driver = {
		.name = "mediatek-mt6763-pinctrl",
		.owner = THIS_MODULE,
		.of_match_table = mt6763_pctrl_match,
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
