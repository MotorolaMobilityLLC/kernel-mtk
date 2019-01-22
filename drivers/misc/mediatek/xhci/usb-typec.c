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
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/atomic.h>
#include <xhci-mtk-driver.h>
#include <typec.h>
#ifdef CONFIG_USB_C_SWITCH_U3_MUX
#include "usb_switch.h"
#include "typec.h"
#endif
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

#ifdef CONFIG_TCPC_CLASS
int tcpc_otg_enable(void)
{
	int ret = 0;

	if (!usbc_otg_attached) {
		ret = mtk_xhci_driver_load(false);
		usbc_otg_attached = true;
	}
	return ret;
}

int tcpc_otg_disable(void)
{
	if (usbc_otg_attached) {
		mtk_xhci_driver_unload(false);
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
			mtk_xhci_enable_vbus();
			tcpc_boost_on = true;
		}
	} else {
		if (tcpc_boost_on) {
			mtk_xhci_disable_vbus();
			tcpc_boost_on = false;
		}
	}
	mutex_unlock(&tcpc_otg_pwr_lock);
}

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
#ifdef CONFIG_USB_C_SWITCH_U3_MUX
			if (noti->typec_state.polarity == 0)
				usb3_switch_ctrl_sel(CC2_SIDE);
			else
				usb3_switch_ctrl_sel(CC1_SIDE);
#endif
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

static int __init rt_typec_init(void)
{
#ifdef CONFIG_TCPC_CLASS
		int ret;
#endif /* CONFIG_TCPC_CLASS */


#ifdef CONFIG_USB_C_SWITCH
#ifndef CONFIG_TCPC_CLASS
		typec_host_driver.priv_data = NULL;
		register_typec_switch_callback(&typec_host_driver);
#endif /* if not CONFIG_TCPC_CLASS */
#endif

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

late_initcall(rt_typec_init);

static void __exit rt_typec_init_cleanup(void)
{
}

module_exit(rt_typec_init_cleanup);

