/*
 * Copyright (C) 2016 MediaTek, Inc.
 *
 * Author: Chen Zhong <chen.zhong@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/wakelock.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/regmap.h>
#include <linux/mfd/mt6323/registers.h>
#include <linux/mfd/mt6397/registers.h>
#include <linux/mfd/mt6397/core.h>
#include <linux/slab.h>
#include <linux/irqdomain.h>

#define PMIC_KEYS_ID_PWR	0
#define PMIC_KEYS_ID_HOME	1

struct pmic_keys_regs {
	u32 deb_reg;
	u32 deb_mask;
	u32 intsel_reg;
	u32 intsel_mask;
};

#define PMIC_KEYS_REGS(_deb_reg, _deb_mask, _intsel_reg, _intsel_mask)	\
{									\
	.deb_reg		= _deb_reg,				\
	.deb_mask		= _deb_mask,				\
	.intsel_reg		= _intsel_reg,				\
	.intsel_mask		= _intsel_mask,				\
}

static const struct pmic_keys_regs mt6397_keys_regs[] = {
	[PMIC_KEYS_ID_PWR] = PMIC_KEYS_REGS(MT6397_CHRSTATUS, 0x8, MT6397_INT_RSV, 0x10),
	[PMIC_KEYS_ID_HOME] = PMIC_KEYS_REGS(MT6397_OCSTATUS2, 0x10, MT6397_INT_RSV, 0x8),
};

static const struct pmic_keys_regs mt6323_keys_regs[] = {
	[PMIC_KEYS_ID_PWR] = PMIC_KEYS_REGS(MT6323_CHRSTATUS, 0x2, MT6323_INT_MISC_CON, 0x10),
	[PMIC_KEYS_ID_HOME] = PMIC_KEYS_REGS(MT6323_CHRSTATUS, 0x4, MT6323_INT_MISC_CON, 0x8),
};

struct pmic_keys_info {
	struct mtk_pmic_keys *keys;
	const struct pmic_keys_regs *regs;
	int keycode;
	int irq;
	u32 hw_irq;
};

struct mtk_pmic_keys {
	struct input_dev *input_dev;
	struct device *dev;
	struct regmap *regmap;
	struct irq_domain *irq_domain;
	struct pmic_keys_info pwrkey, homekey;
};

static irqreturn_t mtk_pmic_keys_irq_handler_thread(int irq, void *data)
{
	struct pmic_keys_info *info = data;
	u32 key_deb, pressed;

	regmap_read(info->keys->regmap, info->regs->deb_reg, &key_deb);

	key_deb &= info->regs->deb_mask;

	pressed = !key_deb;

	input_report_key(info->keys->input_dev, info->keycode, pressed);
	input_sync(info->keys->input_dev);

	dev_info(info->keys->dev, "[PMICKEYS] (%s) key =%d using PMIC\n",
		pressed ? "pressed" : "released", info->keycode);

	return IRQ_HANDLED;
}

static int mtk_pmic_key_setup(struct mtk_pmic_keys *keys,
		const char *propname, struct pmic_keys_info *info, bool wakeup)
{
	int ret;

	ret = of_property_read_u32(keys->dev->of_node, propname, &info->keycode);
	if (ret)
		return 0;

	if (!info->keycode)
		return 0;

	info->keys = keys;

	ret = regmap_update_bits(keys->regmap, info->regs->intsel_reg,
			info->regs->intsel_mask, info->regs->intsel_mask);
	if (ret < 0)
		return ret;

	info->irq = irq_create_mapping(keys->irq_domain, info->hw_irq);
	if (info->irq <= 0)
		return -EINVAL;

	ret = devm_request_threaded_irq(keys->dev, info->irq, NULL,
				   mtk_pmic_keys_irq_handler_thread,
				   IRQF_ONESHOT | IRQF_TRIGGER_HIGH,
				   "mtk-pmic-keys", info);
	if (ret) {
		dev_err(keys->dev, "Failed to request IRQ: %d: %d\n",
			info->irq, ret);
		return ret;
	}

	if (wakeup)
		irq_set_irq_wake(info->irq, 1);

	__set_bit(info->keycode, keys->input_dev->keybit);

	return 0;
}

static void mtk_pmic_keys_dispose_irq(struct mtk_pmic_keys *keys)
{
	if (keys->pwrkey.irq)
		irq_dispose_mapping(keys->pwrkey.irq);

	if (keys->homekey.irq)
		irq_dispose_mapping(keys->homekey.irq);
}

static const struct of_device_id of_pmic_keys_match_tbl[] = {
	{
		.compatible = "mediatek,mt6397-keys",
		.data = &mt6397_keys_regs,
	}, {
		.compatible = "mediatek,mt6323-keys",
		.data = &mt6323_keys_regs,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, of_pmic_keys_match_tbl);

static int mtk_pmic_keys_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *res;
	struct mt6397_chip *pmic_chip = dev_get_drvdata(pdev->dev.parent);
	struct mtk_pmic_keys *keys;
	const struct pmic_keys_regs *keys_regs;
	struct input_dev *input_dev;
	const struct of_device_id *of_id =
		of_match_device(of_pmic_keys_match_tbl, &pdev->dev);

	keys = devm_kzalloc(&pdev->dev, sizeof(struct mtk_pmic_keys), GFP_KERNEL);
	if (!keys)
		return -ENOMEM;

	keys->dev = &pdev->dev;
	keys->regmap = pmic_chip->regmap;
	keys->irq_domain = pmic_chip->irq_domain;

	keys_regs = of_id->data;
	keys->pwrkey.regs = &keys_regs[PMIC_KEYS_ID_PWR];
	keys->homekey.regs = &keys_regs[PMIC_KEYS_ID_HOME];

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(&pdev->dev, "no IRQ resource\n");
		return -ENODEV;
	}

	keys->pwrkey.hw_irq = res->start;
	keys->homekey.hw_irq = res->end;

	keys->input_dev = input_dev = devm_input_allocate_device(keys->dev);
	if (!input_dev) {
		dev_err(&pdev->dev, "[PMICKEYS] input allocate device fail.\n");
		return -ENOMEM;
	}

	input_dev->name = "mtk-pmic-keys";
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0001;
	input_dev->dev.parent = &pdev->dev;

	__set_bit(EV_KEY, input_dev->evbit);

	ret = mtk_pmic_key_setup(keys, "mediatek,pwrkey-code", &keys->pwrkey, true);
	if (ret)
		goto out_dispose_irq;

	ret = mtk_pmic_key_setup(keys, "mediatek,homekey-code", &keys->homekey, false);
	if (ret)
		goto out_dispose_irq;

	ret = input_register_device(input_dev);
	if (ret) {
		dev_err(&pdev->dev, "[PMICKEYS] register input device failed (%d)\n", ret);
		input_free_device(input_dev);
		return ret;
	}

	input_set_drvdata(input_dev, keys);

	return 0;

out_dispose_irq:
	mtk_pmic_keys_dispose_irq(keys);
	return ret;
}

static int mtk_pmic_keys_remove(struct platform_device *pdev)
{
	struct mtk_pmic_keys *keys = platform_get_drvdata(pdev);

	mtk_pmic_keys_dispose_irq(keys);

	input_unregister_device(keys->input_dev);

	return 0;
}

static struct platform_driver pmic_keys_pdrv = {
	.probe = mtk_pmic_keys_probe,
	.remove = mtk_pmic_keys_remove,
	.driver = {
		   .name = "mtk-pmic-keys",
		   .owner = THIS_MODULE,
		   .of_match_table = of_pmic_keys_match_tbl,
	},
};

module_platform_driver(pmic_keys_pdrv);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Chen Zhong <chen.zhong@mediatek.com>");
MODULE_DESCRIPTION("MTK pmic-keys driver v0.1");
MODULE_ALIAS("keypad:pmic");
