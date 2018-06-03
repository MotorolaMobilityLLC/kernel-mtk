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

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include <linux/irq.h>
#include <linux/switch.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/jiffies.h>
#include <linux/list.h>
#include <linux/atomic.h>

#include <xhci-mtk-driver.h>
#ifdef CONFIG_PROJECT_PHY
#include <mtk-phy-asic.h>
#endif

#if CONFIG_MTK_GAUGE_VERSION == 30
#include <mt-plat/mtk_battery.h>
#else
#include <mt-plat/battery_meter.h>
#endif

#include <mt-plat/charger_class.h>
#ifdef CONFIG_USB_XHCI_MTK_SUSPEND_SUPPORT
#include <mtk_sleep.h>
#endif

#define RET_SUCCESS 0
#define RET_FAIL 1


#if CONFIG_MTK_GAUGE_VERSION == 30
static struct charger_device *primary_charger;
#endif

static struct wake_lock mtk_xhci_wakelock;

enum dualrole_state {
	DUALROLE_DEVICE,
	DUALROLE_HOST,
};

static enum dualrole_state mtk_dualrole_stat = DUALROLE_DEVICE;
static struct switch_dev mtk_otg_state;
static bool boost_on;

u32 xhci_debug_level = K_ALET | K_CRIT | K_ERR | K_WARNIN;

module_param(xhci_debug_level, int, 0644);

bool mtk_is_charger_4_vol(void)
{
#if defined(CONFIG_USBIF_COMPLIANCE) || defined(CONFIG_POWER_EXT)
	return false;
#else
	int vol =  battery_meter_get_charger_voltage();

	mtk_xhci_mtk_printk(K_DEBUG, "voltage(%d)\n", vol);

	return (vol > 4000) ? true : false;
#endif
}

static void mtk_enable_otg_mode(void)
{
	boost_on = true;
#if CONFIG_MTK_GAUGE_VERSION == 30
	charger_dev_enable_otg(primary_charger, true);
	charger_dev_set_boost_current_limit(primary_charger, 1500000);
	charger_dev_kick_wdt(primary_charger);
	enable_boost_polling(true);
#else
	set_chr_enable_otg(0x1);
	set_chr_boost_current_limit(1500);
#endif
}

static void mtk_disable_otg_mode(void)
{
#if CONFIG_MTK_GAUGE_VERSION == 30
	charger_dev_enable_otg(primary_charger, false);
	enable_boost_polling(false);
#else
	set_chr_enable_otg(0x0);
#endif
	boost_on = false;
}

static int mtk_xhci_hcd_init(void)
{
	int retval;

	retval = xhci_mtk_register_plat();
	if (retval < 0) {
		pr_err("Problem registering platform driver.\n");
		return retval;
	}

	return 0;
}

#if CONFIG_MTK_GAUGE_VERSION == 30

struct usbotg_boost_manager {
	struct platform_device *pdev;
	struct gtimer otg_kthread_gtimer;
	struct workqueue_struct *otg_boost_workq;
	struct work_struct kick_work;
	struct charger_device *primary_charger;
	unsigned int polling_interval;
	bool polling_enabled;
};
static struct usbotg_boost_manager *g_info;


void enable_boost_polling(bool poll_en)
{
	if (g_info) {
		if (poll_en) {
			gtimer_start(&g_info->otg_kthread_gtimer, g_info->polling_interval);
			g_info->polling_enabled = true;
		} else {
			g_info->polling_enabled = false;
			gtimer_stop(&g_info->otg_kthread_gtimer);
		}
	}
}

static void usbotg_boost_kick_work(struct work_struct *work)
{

	struct usbotg_boost_manager *usb_boost_manager =
		container_of(work, struct usbotg_boost_manager, kick_work);

	mtk_xhci_mtk_printk(K_ALET, "usbotg_boost_kick_work\n");

	charger_dev_kick_wdt(usb_boost_manager->primary_charger);

	if (usb_boost_manager->polling_enabled == true)
		gtimer_start(&usb_boost_manager->otg_kthread_gtimer, usb_boost_manager->polling_interval);
}

static int usbotg_gtimer_func(struct gtimer *data)
{
	struct usbotg_boost_manager *usb_boost_manager =
		container_of(data, struct usbotg_boost_manager, otg_kthread_gtimer);

	queue_work(usb_boost_manager->otg_boost_workq, &usb_boost_manager->kick_work);
	return 0;
}

static int usbotg_boost_manager_probe(struct platform_device *pdev)
{
	struct usbotg_boost_manager *info = NULL;
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;

	info = devm_kzalloc(&pdev->dev, sizeof(struct usbotg_boost_manager), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	platform_set_drvdata(pdev, info);
	info->pdev = pdev;
	info->primary_charger = get_charger_by_name("primary_chg");
	if (!info->primary_charger) {
		pr_info("%s: get primary charger device failed\n", __func__);
		return -ENODEV;
	}

	gtimer_init(&info->otg_kthread_gtimer, &info->pdev->dev, "otg_boost");
	info->otg_kthread_gtimer.callback = usbotg_gtimer_func;
	if (of_property_read_u32(node, "boost_period", (u32 *) &info->polling_interval))
		return -EINVAL;

	info->polling_interval = 30;
	info->otg_boost_workq = create_singlethread_workqueue("otg_boost_workq");
	INIT_WORK(&info->kick_work, usbotg_boost_kick_work);
	g_info = info;
	return 0;
}

static int usbotg_boost_manager_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id usbotg_boost_manager_of_match[] = {
	{.compatible = "mediatek,usb_boost_manager"},
	{},
};
MODULE_DEVICE_TABLE(of, usbotg_boost_manager_of_match);
static struct platform_driver boost_manager_driver = {
	.remove = usbotg_boost_manager_remove,
	.probe = usbotg_boost_manager_probe,
	.driver = {
		   .name = "usb_boost_manager",
		   .of_match_table = usbotg_boost_manager_of_match,
		   },
};

#endif



#ifdef CONFIG_USBIF_COMPLIANCE

#else
static void mtk_xhci_wakelock_init(void)
{
	wake_lock_init(&mtk_xhci_wakelock, WAKE_LOCK_SUSPEND, "xhci.wakelock");
}
#endif

void mtk_xhci_wakelock_lock(void)
{
	if (!wake_lock_active(&mtk_xhci_wakelock))
		wake_lock(&mtk_xhci_wakelock);
	mtk_xhci_mtk_printk(K_DEBUG, "xhci_wakelock_lock done\n");
}

void mtk_xhci_wakelock_unlock(void)
{
	if (wake_lock_active(&mtk_xhci_wakelock))
		wake_unlock(&mtk_xhci_wakelock);
	mtk_xhci_mtk_printk(K_DEBUG, "xhci_wakelock_unlock done\n");
}

static void mtk_xhci_hcd_cleanup(void)
{
	xhci_mtk_unregister_plat();
}
static struct delayed_work host_plug_test_work;
static int host_plug_test_enable; /* default disable */
module_param(host_plug_test_enable, int, 0644);
static int host_plug_in_test_period_ms = 5000;
module_param(host_plug_in_test_period_ms, int, 0644);
static int host_plug_out_test_period_ms = 5000;
module_param(host_plug_out_test_period_ms, int, 0644);
static int host_test_vbus_off_time_us = 3000;
module_param(host_test_vbus_off_time_us, int, 0644);
static int host_test_vbus_only = 1;
module_param(host_test_vbus_only, int, 0644);
static int host_plug_test_triggered;
static int host_req;
static struct wake_lock host_test_wakelock;
static int _mtk_xhci_driver_load(bool vbus_on);
static void _mtk_xhci_driver_unload(bool vbus_off);
static void do_host_plug_test_work(struct work_struct *data)
{
	static ktime_t ktime_begin, ktime_end;
	static s64 diff_time;
	static int host_on;
	static int wake_lock_inited;

	if (!wake_lock_inited) {
		mtk_xhci_mtk_printk(K_ALET, "%s wake_lock_init\n", __func__);
		wake_lock_init(&host_test_wakelock, WAKE_LOCK_SUSPEND, "host.test.lock");
		wake_lock_inited = 1;
	}

	host_plug_test_triggered = 1;
	/* sync global status */
	mb();
	wake_lock(&host_test_wakelock);
	mtk_xhci_mtk_printk(K_ALET, "BEGIN");
	ktime_begin = ktime_get();

	host_on  = 1;
	while (1) {
		if (!host_req && host_on) {
			mtk_xhci_mtk_printk(K_ALET, "about to exit, host_on<%d>\n", host_on);
			break;
		}
		msleep(50);

		ktime_end = ktime_get();
		diff_time = ktime_to_ms(ktime_sub(ktime_end, ktime_begin));
		if (host_on && diff_time >= host_plug_in_test_period_ms) {
			host_on = 0;
			mtk_xhci_mtk_printk(K_ALET, "OFF\n");

			ktime_begin = ktime_get();

			/* simulate plug out */
			mtk_disable_otg_mode();
			udelay(host_test_vbus_off_time_us);

			if (!host_test_vbus_only)
				_mtk_xhci_driver_unload(true);
		} else if (!host_on && diff_time >= host_plug_out_test_period_ms) {
			host_on = 1;
			mtk_xhci_mtk_printk(K_ALET, "ON\n");

			ktime_begin = ktime_get();
			if (!host_test_vbus_only)
				_mtk_xhci_driver_load(true);

			mtk_enable_otg_mode();
			msleep(100);
		}
	}

	/* from ON to OFF state */
	msleep(1000);
	_mtk_xhci_driver_unload(true);

	host_plug_test_triggered = 0;
	wake_unlock(&host_test_wakelock);
	mtk_xhci_mtk_printk(K_ALET, "END\n");
}


static int _mtk_xhci_driver_load(bool vbus_on)
{
	int ret = 0;

	/* recover clock/power setting and deassert reset bit of mac */
#ifdef CONFIG_PROJECT_PHY
	usb_phy_recover(0);
	usb20_pll_settings(true, true);
#endif
	ret = mtk_xhci_hcd_init();
	if (ret) {
		ret = -ENXIO;
		goto _err;
	}
#ifdef CONFIG_USB_XHCI_MTK_SUSPEND_SUPPORT
	slp_set_infra_on(true);
#else
	mtk_xhci_wakelock_lock();
#endif
#ifndef CONFIG_USBIF_COMPLIANCE
	switch_set_state(&mtk_otg_state, 1);
#endif
	mtk_dualrole_stat = DUALROLE_HOST;

	if (vbus_on)
		mtk_enable_otg_mode();

	if (host_plug_test_enable && !host_plug_test_triggered)
		schedule_delayed_work(&host_plug_test_work, 0);

	return 0;

_err:
#ifdef CONFIG_PROJECT_PHY
	usb_phy_savecurrent(1);
#endif
	mtk_dualrole_stat = DUALROLE_DEVICE;

	return ret;
}

static void _mtk_xhci_driver_unload(bool vbus_off)
{
	mtk_xhci_hcd_cleanup();

	if (vbus_off)
		mtk_disable_otg_mode();

	/* close clock/power setting and assert reset bit of mac */
#ifdef CONFIG_PROJECT_PHY
	usb20_pll_settings(true, false);
	usb_phy_savecurrent(1);
#endif

#ifndef CONFIG_USBIF_COMPLIANCE
	switch_set_state(&mtk_otg_state, 0);
#endif
#ifdef CONFIG_USB_XHCI_MTK_SUSPEND_SUPPORT
	slp_set_infra_on(false);
#endif
	mtk_xhci_wakelock_unlock();
	mtk_dualrole_stat = DUALROLE_DEVICE;
}

int mtk_xhci_driver_load(bool vbus_on)
{
	static int host_plug_test_work_inited;

	host_req = 1;

	if (!host_plug_test_work_inited) {
		INIT_DELAYED_WORK(&host_plug_test_work, do_host_plug_test_work);
		host_plug_test_work_inited = 1;
	}

	if (host_plug_test_triggered)
		return 0;

	return _mtk_xhci_driver_load(vbus_on);
}
void mtk_xhci_driver_unload(bool vbus_off)
{
	host_req = 0;

	if (host_plug_test_triggered)
		return;

	_mtk_xhci_driver_unload(vbus_off);
}

void mtk_xhci_disable_vbus(void)
{
	mtk_disable_otg_mode();
}

void mtk_xhci_enable_vbus(void)
{
	mtk_enable_otg_mode();
}

static void mtk_xhci_switch_init(void)
{
	mtk_otg_state.name = "otg_state";
	mtk_otg_state.index = 0;
	mtk_otg_state.state = 0;

#ifndef CONFIG_USBIF_COMPLIANCE
	if (switch_dev_register(&mtk_otg_state))
		mtk_xhci_mtk_printk(K_DEBUG, "switch_dev_register fail\n");
	else
		mtk_xhci_mtk_printk(K_DEBUG, "switch_dev register success\n");
#endif
}

bool mtk_is_host_mode(void)
{
#ifdef CONFIG_TCPC_CLASS
	return boost_on;
#else
	return (mtk_dualrole_stat == DUALROLE_HOST) ? true : false;
#endif /* CONFIG_TCPC_CLASS */
}


#ifdef CONFIG_USBIF_COMPLIANCE

#else
static int __init xhci_hcd_init(void)
{
	mtk_xhci_wakelock_init();
	mtk_xhci_switch_init();

#if CONFIG_MTK_GAUGE_VERSION == 30
	primary_charger = get_charger_by_name("primary_chg");
	if (!primary_charger) {
		pr_err("%s: get primary charger device failed\n", __func__);
		return -ENODEV;
	}
	platform_driver_register(&boost_manager_driver);
#endif

	return 0;
}

late_initcall(xhci_hcd_init);

static void __exit xhci_hcd_cleanup(void)
{
}

module_exit(xhci_hcd_cleanup);
#endif

