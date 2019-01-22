/*
 *  mtk_auxadc_intf.c
 *  Mediatek PMIC AUXADC Interface Driver
 *
 *  Copyright (C) 2015 Richtek Technology Corp.
 *  Jeff Chang <jeff_chang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <mt-plat/mtk_auxadc_intf.h>

#ifdef CONFIG_MTK_PMIC_CHIP_MT6336
#include "include/mt6336/mt6336.h"

struct mt6336_ctrl *auxadc_intf_ctrl;
#endif

static struct mtk_auxadc_intf *auxadc_intf;

const char *pmic_auxadc_channel_name[] = {
#ifdef CONFIG_MTK_PMIC_CHIP_MT6335
	/* mt6335 */
	"BATADC",
	"VCDT",
	"BAT TEMP",
	"BATID",
	"VBIF",
	"MT6335 CHIP TEMP",
	"DCXO",
	"TSX",
#endif
#ifdef CONFIG_MTK_PMIC_CHIP_MT6336
	/* mt6336 */
	"BATSNS",
	"VBUS",
	"BAT TEMP",
	"MT6336 CHIP TEMP",
	"TYPEC CC DETECT",
	"BATID",
	"VLED1",
	"VLED2",
#endif
#ifdef CONFIG_MTK_PMIC_CHIP_MT6337
	/* mt6337 */
	"BATSNS",
	"MT6337 CHIP TEMP",
	"ACCDET",
	"HPOPS_CAL",
#endif
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
	/* mt6355 */
	"BATADC",
	"VCDT",
	"BAT TEMP",
	"BATID",
	"VBIF",
	"MT6355 CHIP TEMP",
	"DCXO",
	"ACCDET",
	"TSX",
	"HP",
#endif
#ifdef CONFIG_MTK_PMIC_CHIP_MT6356
	/* mt6356 */
	"BATADC",
	"VCDT",
	"BAT TEMP",
	"BATID",
	"VBIF",
	"MT6356 CHIP TEMP",
	"DCXO",
	"ACCDET",
	"TSX",
	"HP",
	"ISENSE",
	"BUCK1_TEMP",
	"BUCK2_TEMP",
#endif
};
#define PMIC_AUXADC_CHANNEL_MAX	ARRAY_SIZE(pmic_auxadc_channel_name)

const char *pmic_get_auxadc_name(u8 list)
{
	return pmic_auxadc_channel_name[list];
}
EXPORT_SYMBOL(pmic_get_auxadc_name);

int pmic_get_auxadc_value(u8 list)
{
	int value = 0;
#ifdef CONFIG_MTK_PMIC_CHIP_MT6335
	if (list >= AUXADC_LIST_MT6335_START &&
				list <= AUXADC_LIST_MT6335_END) {
		value = mt6335_get_auxadc_value(list);
		return value;
	}
#endif /* CONFIG_MTK_PMIC_CHIP_MT6335 */
#ifdef CONFIG_MTK_PMIC_CHIP_MT6336
	if (list >= AUXADC_LIST_MT6336_START &&
			list <= AUXADC_LIST_MT6336_END) {
		mt6336_ctrl_enable(auxadc_intf_ctrl);
		value = mt6336_get_auxadc_value(list);
		mt6336_ctrl_disable(auxadc_intf_ctrl);
		return value;
	}
#endif /* CONFIG_MTK_PMIC_CHIP_MT6336 */
#ifdef CONFIG_MTK_PMIC_CHIP_MT6337
	if (list >= AUXADC_LIST_MT6337_START &&
			list <= AUXADC_LIST_MT6337_END) {
		value = mt6337_get_auxadc_value(list);
		return value;
	}
#endif /* CONFIG_MTK_PMIC_CHIP_MT6337 */
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
	if (list >= AUXADC_LIST_MT6355_START &&
			list <= AUXADC_LIST_MT6355_END) {
		value = mt6355_get_auxadc_value(list);
		return value;
	}
#endif /* CONFIG_MTK_PMIC_CHIP_MT6355 */
#ifdef CONFIG_MTK_PMIC_CHIP_MT6356
	if (list >= AUXADC_LIST_MT6356_START &&
			list <= AUXADC_LIST_MT6356_END) {
		value = mt6356_get_auxadc_value(list);
		return value;
	}
#endif /* CONFIG_MTK_PMIC_CHIP_MT6356 */
	pr_err("%s Invalid AUXADC LIST\n", __func__);
	return -EINVAL;
}

void pmic_auxadc_dump_regs(char *buf)
{
	snprintf(buf+strlen(buf), 1024, "====%s====\n", __func__);
#ifdef CONFIG_MTK_PMIC_CHIP_MT6335
	mt6335_auxadc_dump_regs(buf);
#endif /* CONFIG_MTK_PMIC_CHIP_MT6335*/
#ifdef CONFIG_MTK_PMIC_CHIP_MT6336
	mt6336_auxadc_dump_regs(buf);
#endif /* CONFIG_MTK_PMIC_CHIP_MT6336 */
#ifdef CONFIG_MTK_PMIC_CHIP_MT6337
	mt6337_auxadc_dump_regs(buf);
#endif /* CONFIG_MTK_PMIC_CHIP_MT6337 */
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
	mt6355_auxadc_dump_regs(buf);
#endif /* CONFIG_MTK_PMIC_CHIP_MT6355 */
#ifdef CONFIG_MTK_PMIC_CHIP_MT6356
	mt6356_auxadc_dump_regs(buf);
#endif /* CONFIG_MTK_PMIC_CHIP_MT6356 */
}

static struct mtk_auxadc_ops pmic_auxadc_ops = {
	.get_channel_value = pmic_get_auxadc_value,
	.dump_regs = pmic_auxadc_dump_regs,
};

static struct mtk_auxadc_intf pmic_auxadc_intf = {
	.ops = &pmic_auxadc_ops,
	.name = "mtk_auxadc_intf",
	.channel_num = PMIC_AUXADC_CHANNEL_MAX,
	.channel_name = pmic_auxadc_channel_name,
};

int register_mtk_auxadc_intf(struct mtk_auxadc_intf *intf)
{
	auxadc_intf = intf;
	platform_device_register_simple(auxadc_intf->name, -1, NULL, 0);
	return 0;
}
EXPORT_SYMBOL(register_mtk_auxadc_intf);

void mtk_auxadc_init(void)
{
#ifdef CONFIG_MTK_PMIC_CHIP_MT6335
	mt6335_auxadc_init();
#endif /* CONFIG_MTK_PMIC_CHIP_MT6335 */
#ifdef CONFIG_MTK_PMIC_CHIP_MT6336
	mt6336_auxadc_init();
#endif /* CONFIG_MTK_PMIC_CHIP_MT6336 */
#ifdef CONFIG_MTK_PMIC_CHIP_MT6337
	mt6337_auxadc_init();
#endif /* CONFIG_MTK_PMIC_CHIP_MT6337 */
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
	mt6355_auxadc_init();
#endif /* CONFIG_MTK_PMIC_CHIP_MT6355 */
#ifdef CONFIG_MTK_PMIC_CHIP_MT6356
	mt6356_auxadc_init();
#endif /* CONFIG_MTK_PMIC_CHIP_MT6356 */

	if (register_mtk_auxadc_intf(&pmic_auxadc_intf) < 0)
		pr_err("[%s] register MTK Auxadc Intf Fail\n", __func__);
}

static ssize_t mtk_auxadc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
static ssize_t mtk_auxadc_show(struct device *dev,
		struct device_attribute *attr, char *buf);
static struct device_attribute mtk_auxadc_attrs[] = {
	__ATTR(dump_channel, 0444, mtk_auxadc_show, NULL),
	__ATTR(channel, 0644, mtk_auxadc_show, mtk_auxadc_store),
	__ATTR(value, 0444, mtk_auxadc_show, NULL),
	__ATTR(regs, 0444, mtk_auxadc_show, NULL),
	__ATTR(md_channel, 0644, mtk_auxadc_show, mtk_auxadc_store),
	__ATTR(md_value, 0444, mtk_auxadc_show, NULL),
	__ATTR(md_dump_channel, 0444, mtk_auxadc_show, NULL),
};

static int get_parameters(char *buf, long int *param, int num_of_par)
{
	char *token;
	int base, cnt;

	token = strsep(&buf, " ");
	for (cnt = 0; cnt < num_of_par; cnt++) {
		if (token != NULL) {
			if ((token[1] == 'x') || (token[1] == 'X'))
				base = 16;
			else
				base = 10;
			if (kstrtoul(token, base, &param[cnt]) != 0)
				return -EINVAL;
			token = strsep(&buf, " ");
		} else
			return -EINVAL;
	}
	return 0;
}

static ssize_t mtk_auxadc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	ptrdiff_t cmd;
	int ret;
	long int val;

	cmd = attr - mtk_auxadc_attrs;
	switch (cmd) {
	case AUXADC_CHANNEL:
		ret = get_parameters((char *)buf, &val, 1);
		if (ret < 0) {
			dev_err(dev, "get parameter fail\n");
			return -EINVAL;
		}
		auxadc_intf->dbg_chl = val;
		break;
	default:
		break;

	}
	return count;
}

static ssize_t mtk_auxadc_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ptrdiff_t cmd;
	int i;
	int value;

	cmd = attr - mtk_auxadc_attrs;
	buf[0] = '\0';

	switch (cmd) {
	case AUXADC_DUMP:
		snprintf(buf+strlen(buf),
			256, "=== Channel Dump Start ===\n");
		for (i = 0; i < auxadc_intf->channel_num; i++) {
			value = auxadc_intf->ops->get_channel_value(i);
			if (value >= 0)
				snprintf(buf+strlen(buf), 256,
					"Channel%d (%s) = 0x%x(%d)\n",
					i, auxadc_intf->channel_name[i],
					value, value);
		}
		snprintf(buf+strlen(buf),
			256, "==== Channel Dump End ====\n");
		break;
	case AUXADC_CHANNEL:
		snprintf(buf + strlen(buf), 256, "%d (%s)\n",
				auxadc_intf->dbg_chl,
				auxadc_intf->
				channel_name[auxadc_intf->dbg_chl]);
		break;
	case AUXADC_VALUE:
		value = auxadc_intf->ops->
			get_channel_value(auxadc_intf->dbg_chl);
		snprintf(buf + strlen(buf),
			256, "Channel%d (%s) = 0x%x(%d)\n",
			auxadc_intf->dbg_chl,
			auxadc_intf->channel_name[auxadc_intf->dbg_chl],
				value, value);
		break;
	case AUXADC_REGS:
		auxadc_intf->ops->dump_regs(buf);
		break;
	default:
		break;
	}
	return strlen(buf);
}

static int create_sysfs_interface(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mtk_auxadc_attrs); i++)
		if (device_create_file(dev, mtk_auxadc_attrs + i))
			return -EINVAL;
	return 0;
}

static int remove_sysfs_interface(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mtk_auxadc_attrs); i++)
		device_remove_file(dev, mtk_auxadc_attrs + i);
	return 0;
}

static int mtk_auxadc_intf_probe(struct platform_device *pdev)
{
	int ret;

	pr_info("%s\n", __func__);

	platform_set_drvdata(pdev, auxadc_intf);
	ret = create_sysfs_interface(&pdev->dev);
	if (ret < 0) {
		pr_err("%s create sysfs fail\n", __func__);
		return -EINVAL;
	}
#ifdef CONFIG_MTK_PMIC_CHIP_MT6336
	auxadc_intf_ctrl = mt6336_ctrl_get("auxadc_intf");
#endif /* CONFIG_MTK_PMIC_CHIP_MT6336 */

	return 0;
}

static int mtk_auxadc_intf_remove(struct platform_device *pdev)
{
	int ret;

	ret = remove_sysfs_interface(&pdev->dev);
	return 0;
}

static struct platform_driver mtk_auxadc_intf_driver = {
	.driver = {
		.name = "mtk_auxadc_intf",
	},
	.probe = mtk_auxadc_intf_probe,
	.remove = mtk_auxadc_intf_remove,
};
module_platform_driver(mtk_auxadc_intf_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MTK PMIC AUXADC Interface Driver");
MODULE_AUTHOR("Jeff Chang <jeff_chang@richtek.com>");
MODULE_VERSION("1.0.0_M");
