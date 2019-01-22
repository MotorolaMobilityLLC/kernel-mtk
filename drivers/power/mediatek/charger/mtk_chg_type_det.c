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

#include <generated/autoconf.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/wakelock.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/writeback.h>
#include <linux/seq_file.h>
#include <linux/power_supply.h>
#include <linux/time.h>
#include <linux/uaccess.h>

#include <mt-plat/mtk_battery.h>
#include <mt-plat/upmu_common.h>
#include <mach/upmu_sw.h>
#include <mach/upmu_hw.h>
#include <mt-plat/mtk_boot.h>
#include <mt-plat/charger_type.h>
#include <pmic.h>


static CHARGER_TYPE g_chr_type;
static struct mt_charger *g_mt_charger;

#ifdef CONFIG_MTK_FPGA
/*****************************************************************************
 * FPGA
 ******************************************************************************/
int hw_charging_get_charger_type(void)
{
	return STANDARD_HOST;
}

#else
/*****************************************************************************
 * EVB / Phone
 ******************************************************************************/

static const char * const mtk_chg_type_name[] = {
	"Charger Unknown",
	"Standard USB Host",
	"Charging USB Host",
	"Non-standard Charger",
	"Standard Charger",
	"Apple 2.1A Charger",
	"Apple 1.0A Charger",
	"Apple 0.5A Charger",
	"Wireless Charger",
};

static void dump_charger_name(CHARGER_TYPE type)
{
	switch (type) {
	case CHARGER_UNKNOWN:
	case STANDARD_HOST:
	case CHARGING_HOST:
	case NONSTANDARD_CHARGER:
	case STANDARD_CHARGER:
	case APPLE_2_1A_CHARGER:
	case APPLE_1_0A_CHARGER:
	case APPLE_0_5A_CHARGER:
		pr_err("%s: charger type: %d, %s\n", __func__, type,
			mtk_chg_type_name[type]);
		break;
	default:
		pr_err("%s: charger type: %d, Not Defined!!!\n", __func__,
			type);
		break;
	}
}

/************************************************/
/* Power Supply
*************************************************/

struct mt_charger {
	struct power_supply_desc charger;
	struct power_supply *charger_psy;
	struct power_supply_desc ac;
	struct power_supply *ac_psy;
	struct power_supply_desc usb;
	struct power_supply *usb_psy;
	bool chg_online; /* Has charger in or not */
};

static int mt_charger_online(bool online)
{
	int ret = 0;

#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
	if (!online) {
		int boot_mode = 0;

		boot_mode = get_boot_mode();

		if (boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT ||
		    boot_mode == LOW_POWER_OFF_CHARGING_BOOT) {
			pr_err("%s: Unplug Charger/USB\n", __func__);
			mt_power_off();
		}
	}
#endif

	return ret;
}

/************************************************/
/* Power Supply Functions
*************************************************/

static int mt_charger_get_property(struct power_supply *psy,
	enum power_supply_property psp, union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 0;
		/* Force to 1 in all charger type */
		if (g_chr_type != CHARGER_UNKNOWN)
			val->intval = 1;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}


static int mt_charger_set_property(struct power_supply *psy,
	enum power_supply_property psp, const union power_supply_propval *val)
{
	pr_info("%s\n", __func__);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		mt_charger_online(val->intval);
		g_mt_charger->chg_online = val->intval;
		return 0;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		g_chr_type = val->intval;
		break;
	default:
		return -EINVAL;
	}

	dump_charger_name(g_chr_type);

	/* usb */
	if ((g_chr_type == STANDARD_HOST) || (g_chr_type == CHARGING_HOST))
		mt_usb_connect();
	else
		mt_usb_disconnect();

	mtk_charger_int_handler();
	fg_charger_in_handler();

	if (g_mt_charger) {
		power_supply_changed(g_mt_charger->ac_psy);
		power_supply_changed(g_mt_charger->usb_psy);
	}

	return 0;
}

static int mt_ac_get_property(struct power_supply *psy,
	enum power_supply_property psp, union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 0;
		/* Force to 1 in all charger type */
		if (g_chr_type != CHARGER_UNKNOWN)
			val->intval = 1;
		/* Reset to 0 if charger type is USB */
		if ((g_chr_type == STANDARD_HOST) || (g_chr_type == CHARGING_HOST))
			val->intval = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int mt_usb_get_property(struct power_supply *psy,
	enum power_supply_property psp, union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if ((g_chr_type == STANDARD_HOST) || (g_chr_type == CHARGING_HOST))
			val->intval = 1;
		else
			val->intval = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static enum power_supply_property mt_charger_properties[] = {
	POWER_SUPPLY_PROP_CHARGE_TYPE,
};

static enum power_supply_property mt_ac_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property mt_usb_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static int mt_charger_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct mt_charger *mt_chg = NULL;

	mt_chg = devm_kzalloc(&pdev->dev, sizeof(struct mt_charger), GFP_KERNEL);
	if (!mt_chg)
		return -ENOMEM;

	mt_chg->chg_online = false;
	mt_chg->charger.name = "charger";
	mt_chg->charger.type = POWER_SUPPLY_TYPE_UNKNOWN;
	mt_chg->charger.properties = mt_charger_properties;
	mt_chg->charger.num_properties = ARRAY_SIZE(mt_charger_properties);
	mt_chg->charger.set_property = mt_charger_set_property;
	mt_chg->charger.get_property = mt_charger_get_property;

	mt_chg->ac.name = "ac";
	mt_chg->ac.type = POWER_SUPPLY_TYPE_MAINS;
	mt_chg->ac.properties = mt_ac_properties;
	mt_chg->ac.num_properties = ARRAY_SIZE(mt_ac_properties);
	mt_chg->ac.get_property = mt_ac_get_property;

	mt_chg->usb.name = "usb";
	mt_chg->usb.type = POWER_SUPPLY_TYPE_USB;
	mt_chg->usb.properties = mt_usb_properties;
	mt_chg->usb.num_properties = ARRAY_SIZE(mt_usb_properties);
	mt_chg->usb.get_property = mt_usb_get_property;

	g_mt_charger = mt_chg;
	mt_chg->charger_psy = power_supply_register(&pdev->dev, &mt_chg->charger, NULL);
	if (IS_ERR(mt_chg->charger_psy)) {
		dev_err(&pdev->dev, "Failed to register power supply: %ld\n",
			PTR_ERR(mt_chg->charger_psy));
		ret = PTR_ERR(mt_chg->charger_psy);
		return ret;
	}

	mt_chg->ac_psy = power_supply_register(&pdev->dev, &mt_chg->ac, NULL);
	if (IS_ERR(mt_chg->ac_psy)) {
		dev_err(&pdev->dev, "Failed to register power supply: %ld\n",
			PTR_ERR(mt_chg->ac_psy));
		ret = PTR_ERR(mt_chg->ac_psy);
		return ret;
	}

	mt_chg->usb_psy = power_supply_register(&pdev->dev, &mt_chg->usb, NULL);
	if (IS_ERR(mt_chg->usb_psy)) {
		dev_err(&pdev->dev, "Failed to register power supply: %ld\n",
			PTR_ERR(mt_chg->usb_psy));
		ret = PTR_ERR(mt_chg->usb_psy);
		return ret;
	}

	platform_set_drvdata(pdev, mt_chg);
	device_init_wakeup(&pdev->dev, 1);

	pr_info("%s\n", __func__);
	return 0;
}

static int mt_charger_remove(struct platform_device *pdev)
{
	struct mt_charger *mt_charger = platform_get_drvdata(pdev);

	power_supply_unregister(mt_charger->charger_psy);
	power_supply_unregister(mt_charger->usb_psy);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mt_charger_suspend(struct device *dev)
{
	/* struct mt_charger *mt_charger = dev_get_drvdata(dev); */
	return 0;
}

static int mt_charger_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mt_charger *mt_charger = platform_get_drvdata(pdev);

	power_supply_changed(mt_charger->charger_psy);
	power_supply_changed(mt_charger->usb_psy);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(mt_charger_pm_ops, mt_charger_suspend,
	mt_charger_resume);

static const struct of_device_id mt_charger_match[] = {
	{ .compatible = "mediatek,mt-charger", },
	{ },
};
static struct platform_driver mt_charger_driver = {
	.probe = mt_charger_probe,
	.remove = mt_charger_remove,
	.driver = {
		.name = "mt-charger-det",
		.owner = THIS_MODULE,
		.pm = &mt_charger_pm_ops,
		.of_match_table = mt_charger_match,
	},
};

/* Legacy api to prevent build error */
bool upmu_is_chr_det(void)
{
	if (upmu_get_rgs_chrdet())
		return true;

	return false;
}

/* Legacy api to prevent build error */
bool pmic_chrdet_status(void)
{
	if (upmu_is_chr_det())
		return true;

	pr_err("%s: No charger\n", __func__);
	return false;
}

CHARGER_TYPE mt_get_charger_type(void)
{
	return g_chr_type;
}

static s32 __init mt_charger_det_init(void)
{
	return platform_driver_register(&mt_charger_driver);
}

static void __exit mt_charger_det_exit(void)
{
	platform_driver_unregister(&mt_charger_driver);
}

subsys_initcall(mt_charger_det_init);
module_exit(mt_charger_det_exit);

MODULE_DESCRIPTION("mt-charger-detection");
MODULE_AUTHOR("MediaTek");
MODULE_LICENSE("GPL v2");

#endif /* CONFIG_MTK_FPGA */
