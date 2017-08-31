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

#ifdef CONFIG_USB_C_SWITCH
#include <typec.h>
#endif

#include <mt-plat/battery_meter.h>

#define RET_SUCCESS 0
#define RET_FAIL 1

#ifdef CONFIG_TCPC_CLASS
#include "tcpm.h"
#include <linux/workqueue.h>
#include <linux/mutex.h>
static struct notifier_block otg_nb;
static bool usbc_otg_attached;
static struct tcpc_device *otg_tcpc_dev;
static struct workqueue_struct *otg_tcpc_power_workq;
static struct workqueue_struct *otg_tcpc_workq;
static struct work_struct tcpc_otg_power_work;
static struct work_struct tcpc_otg_work;
static bool usbc_otg_power_enable;
static bool usbc_otg_enable;
static struct mutex tcpc_otg_lock;
static struct mutex tcpc_otg_pwr_lock;
static bool tcpc_boost_on;
#endif /* CONFIG_TCPC_CLASS */

static struct wake_lock mtk_xhci_wakelock;

enum dualrole_state {
	DUALROLE_DEVICE,
	DUALROLE_HOST,
};

static enum dualrole_state mtk_dualrole_stat = DUALROLE_DEVICE;
static struct switch_dev mtk_otg_state;

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
	set_chr_enable_otg(0x1);
	set_chr_boost_current_limit(1500);
}

static void mtk_disable_otg_mode(void)
{
	set_chr_enable_otg(0x0);
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


#ifdef CONFIG_USBIF_COMPLIANCE

#else
#ifdef CONFIG_USB_C_SWITCH
#ifndef CONFIG_TCPC_CLASS
static int typec_otg_enable(void *data)
{
	pr_info("typec_otg_enable\n");
	return mtk_xhci_driver_load(false);
}

static int typec_otg_disable(void *data)
{
	pr_info("typec_otg_disable\n");
	mtk_xhci_driver_unload(false);
	return 0;
}

static struct typec_switch_data typec_host_driver = {
	.name = "xhci-mtk",
	.type = HOST_TYPE,
	.enable = typec_otg_enable,
	.disable = typec_otg_disable,
};
#endif /* if not CONFIG_TCPC_CLASS */
#endif
#endif

#ifdef CONFIG_USBIF_COMPLIANCE

#else
static void mtk_xhci_wakelock_init(void)
{
	wake_lock_init(&mtk_xhci_wakelock, WAKE_LOCK_SUSPEND, "xhci.wakelock");
#ifdef CONFIG_USB_C_SWITCH
#ifndef CONFIG_TCPC_CLASS
	typec_host_driver.priv_data = NULL;
	register_typec_switch_callback(&typec_host_driver);
#endif /* if not CONFIG_TCPC_CLASS */
#endif
}
#endif

static void mtk_xhci_wakelock_lock(void)
{
	if (!wake_lock_active(&mtk_xhci_wakelock))
		wake_lock(&mtk_xhci_wakelock);
	mtk_xhci_mtk_printk(K_DEBUG, "xhci_wakelock_lock done\n");
}

static void mtk_xhci_wakelock_unlock(void)
{
	if (wake_lock_active(&mtk_xhci_wakelock))
		wake_unlock(&mtk_xhci_wakelock);
	mtk_xhci_mtk_printk(K_DEBUG, "xhci_wakelock_unlock done\n");
}

static void mtk_xhci_hcd_cleanup(void)
{
	xhci_mtk_unregister_plat();
}

int mtk_xhci_driver_load(bool vbus_on)
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
	mtk_xhci_wakelock_lock();
#ifndef CONFIG_USBIF_COMPLIANCE
	switch_set_state(&mtk_otg_state, 1);
#endif
	mtk_dualrole_stat = DUALROLE_HOST;

	if (vbus_on)
		mtk_enable_otg_mode();

	return 0;

_err:
#ifdef CONFIG_PROJECT_PHY
	usb_phy_savecurrent(1);
#endif
	mtk_dualrole_stat = DUALROLE_DEVICE;

	return ret;
}

void mtk_xhci_disable_vbus(void)
{
	mtk_disable_otg_mode();
}

void mtk_xhci_driver_unload(bool vbus_off)
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
	mtk_xhci_wakelock_unlock();
	mtk_dualrole_stat = DUALROLE_DEVICE;
}

void mtk_xhci_switch_init(void)
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
	return tcpc_boost_on;
#else
	return (mtk_dualrole_stat == DUALROLE_HOST) ? true : false;
#endif /* CONFIG_TCPC_CLASS */
}

#ifdef CONFIG_TCPC_CLASS
int tcpc_otg_enable(void)
{
	int ret = 0;

	if (!usbc_otg_attached) {
		/* mtk_idpin_cur_stat = IDPIN_IN_HOST; */
		ret = mtk_xhci_driver_load(false);
		if (!ret) {
			mtk_xhci_wakelock_lock();
			switch_set_state(&mtk_otg_state, 1);
		}
		usbc_otg_attached = true;
	}
	return ret;
}

int tcpc_otg_disable(void)
{
	if (usbc_otg_attached) {
		/* USB PLL Force settings */
		usb20_pll_settings(true, false);
		mtk_xhci_driver_unload(false);
		switch_set_state(&mtk_otg_state, 0);
		mtk_xhci_wakelock_unlock();
		/* mtk_idpin_cur_stat = IDPIN_OUT; */
		usbc_otg_attached = false;
	}
	return 0;
}

static void tcpc_otg_work_call(struct work_struct *work)
{
	bool enable;

	mutex_lock(&tcpc_otg_lock);
	enable = usbc_otg_enable;
	mutex_unlock(&tcpc_otg_lock);

	if (enable)
		tcpc_otg_enable();
	else
		tcpc_otg_disable();
}

static void tcpc_otg_power_work_call(struct work_struct *work)
{
	mutex_lock(&tcpc_otg_pwr_lock);
	if (usbc_otg_power_enable) {
		if (!tcpc_boost_on) {
			mtk_enable_otg_mode();
			tcpc_boost_on = true;
		}
	} else {
		if (tcpc_boost_on) {
			mtk_disable_otg_mode();
			tcpc_boost_on = false;
		}
	}
	mutex_unlock(&tcpc_otg_pwr_lock);
}
#endif /* CONFIG_TCPC_CLASS */

#ifdef CONFIG_TCPC_CLASS
static int otg_tcp_notifier_call(struct notifier_block *nb,
		unsigned long event, void *data)
{
	struct tcp_notify *noti = data;

	switch (event) {
	case TCP_NOTIFY_SOURCE_VBUS:
		pr_info("%s source vbus = %dmv\n",
				__func__, noti->vbus_state.mv);
		mutex_lock(&tcpc_otg_pwr_lock);
		usbc_otg_power_enable = (noti->vbus_state.mv) ? true : false;
		mutex_unlock(&tcpc_otg_pwr_lock);
		queue_work(otg_tcpc_power_workq, &tcpc_otg_power_work);
		break;
	case TCP_NOTIFY_TYPEC_STATE:
		if (noti->typec_state.new_state == TYPEC_ATTACHED_SRC) {
			pr_info("%s OTG Plug in\n", __func__);
			mutex_lock(&tcpc_otg_lock);
			usbc_otg_enable = true;
			mutex_unlock(&tcpc_otg_lock);
		} else if (noti->typec_state.old_state == TYPEC_ATTACHED_SRC &&
				noti->typec_state.new_state == TYPEC_UNATTACHED) {
			pr_info("%s OTG Plug out\n", __func__);
			mutex_lock(&tcpc_otg_lock);
			usbc_otg_enable = false;
			mutex_unlock(&tcpc_otg_lock);
		}
		queue_work(otg_tcpc_workq, &tcpc_otg_work);
		break;
	}
	return NOTIFY_OK;
}
#endif /* CONFIG_TCPC_CLASS */

#ifdef CONFIG_USBIF_COMPLIANCE

#else
static int __init xhci_hcd_init(void)
{
#ifdef CONFIG_TCPC_CLASS
	int ret;
#endif /* CONFIG_TCPC_CLASS */
	mtk_xhci_wakelock_init();
	mtk_xhci_switch_init();
#ifdef CONFIG_TCPC_CLASS
	mutex_init(&tcpc_otg_lock);
	mutex_init(&tcpc_otg_pwr_lock);
	otg_tcpc_workq = create_singlethread_workqueue("tcpc_otg_workq");
	otg_tcpc_power_workq = create_singlethread_workqueue("tcpc_otg_power_workq");
	INIT_WORK(&tcpc_otg_power_work, tcpc_otg_power_work_call);
	INIT_WORK(&tcpc_otg_work, tcpc_otg_work_call);
	otg_tcpc_dev = tcpc_dev_get_by_name("type_c_port0");
	if (!otg_tcpc_dev) {
		pr_err("%s get tcpc device type_c_port0 fail\n", __func__);
		return -ENODEV;
	}

	otg_nb.notifier_call = otg_tcp_notifier_call;
	ret = register_tcp_dev_notifier(otg_tcpc_dev, &otg_nb);
	if (ret < 0) {
		pr_err("%s register tcpc notifer fail\n", __func__);
		return -EINVAL;
	}
#endif /* CONFIG_TCPC_CLASS */
	return 0;
}

late_initcall(xhci_hcd_init);

static void __exit xhci_hcd_cleanup(void)
{
}

module_exit(xhci_hcd_cleanup);
#endif

