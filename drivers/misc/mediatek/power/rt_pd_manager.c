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
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>

#include <mt-plat/mtk_boot.h>
#include <mt-plat/upmu_common.h>
#if CONFIG_MTK_GAUGE_VERSION == 30
#include <mt-plat/mtk_battery.h>
#include <mt-plat/mtk_charger.h>
#else
#include <mt-plat/battery_meter.h>
#include <mt-plat/charging.h>
#endif /* CONFIG_MTK_GAUGE_VERSION */

#include "tcpm.h"
#include "mtk_direct_charge_vdm.h"
#ifdef CONFIG_USB_C_SWITCH_U3_MUX
#include "usb_switch.h"
#include "typec.h"
#endif

#define RT_PD_MANAGER_VERSION	"1.0.5_MTK"

static DEFINE_MUTEX(param_lock);
static DEFINE_MUTEX(tcpc_pe30_rdy_lock);
static DEFINE_MUTEX(tcpc_pd_rdy_lock);
static DEFINE_MUTEX(tcpc_usb_connect_lock);

enum {
	MTK_USB_UNATTACHE,
	MTK_USB_ATTACHED,
};

static struct tcpc_device *tcpc_dev;
static struct notifier_block pd_nb;
static unsigned char pd_state;
static int pd_sink_voltage_new;
static int pd_sink_voltage_old;
static int pd_sink_current_new;
static int pd_sink_current_old;
static unsigned char pd_sink_type;
static bool is_pep30_en_unlock;
static bool tcpc_usb_connected;
static bool tcpc_kpoc;
static unsigned char bc12_chr_type;

#if CONFIG_MTK_GAUGE_VERSION == 30
static struct charger_device *primary_charger;
static struct charger_consumer *chg_consumer;
#endif

static void tcpc_mt_power_off(void)
{
#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
	mt_power_off();
#endif /* CONFIG_MTK_KERNEL_POWER_OFF_CHARGING */
}

enum {
	SINK_TYPE_REMOVE,
	SINK_TYPE_TYPEC,
	SINK_TYPE_PD_TRY,
	SINK_TYPE_PD_CONNECTED,
	SINK_TYPE_REQUEST,
};

int tcpc_is_usb_connect(void)
{
	signed int vbus;

	if (tcpc_dev == NULL) {
		vbus = battery_get_vbus();
		pr_info("%s tcpc_dev is null , vbus:%d\n", __func__, vbus);
		if (vbus < 4300)
			return 0;
		return 1;
	}

	if (!tcpm_get_boot_check_flag(tcpc_dev)) {
		pr_info("%s ta hw %d\n",
			__func__, tcpm_get_ta_hw_exist(tcpc_dev));
		return tcpm_get_ta_hw_exist(tcpc_dev);
	}

	return tcpc_usb_connected ? 1 : 0;
}

bool mtk_is_ta_typec_only(void)
{
	bool stat;

	if (!tcpc_is_usb_connect())
		return false;
	stat = ((pd_state == PD_CONNECT_TYPEC_ONLY_SNK_DFT) ||
		(pd_state == PD_CONNECT_TYPEC_ONLY_SNK)) ? true : false;
	return stat;
}

bool mtk_is_pd_chg_ready(void)
{
	bool ready;

	if (!tcpc_is_usb_connect())
		return false;
	ready = (pd_state == PD_CONNECT_PE_READY_SNK) ? true : false;
	return ready;
}

bool mtk_is_pep30_en_unlock(void)
{
#ifdef CONFIG_RT7207_ADAPTER
	if (!mtk_is_pd_chg_ready())
		return false;
	return is_pep30_en_unlock;
#else
	return false;
#endif /* CONFIG_RT7207_ADAPTER */
}

static int pd_tcp_notifier_call(struct notifier_block *nb,
					unsigned long event, void *data)
{
	struct tcp_notify *noti = data;
	u32 vbus = 0;
	int ret = 0;
#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
	int boot_mode = 0;
#endif /* CONFIG_MTK_KERNEL_POWER_OFF_CHARGING */

	switch (event) {
	case TCP_NOTIFY_PR_SWAP:
		break;
	case TCP_NOTIFY_DR_SWAP:
		break;
	case TCP_NOTIFY_SOURCE_VCONN:
		break;
	case TCP_NOTIFY_VCONN_SWAP:
		break;
	case TCP_NOTIFY_SOURCE_VBUS:
		break;
	case TCP_NOTIFY_SINK_VBUS:
		mutex_lock(&param_lock);
		pd_sink_voltage_new = noti->vbus_state.mv;
		pd_sink_current_new = noti->vbus_state.ma;

		if (noti->vbus_state.type & TCP_VBUS_CTRL_PD_DETECT)
			pd_sink_type = SINK_TYPE_PD_CONNECTED;
		else if (noti->vbus_state.type == TCP_VBUS_CTRL_REMOVE)
			pd_sink_type = SINK_TYPE_REMOVE;
		else if (noti->vbus_state.type == TCP_VBUS_CTRL_TYPEC)
			pd_sink_type = SINK_TYPE_TYPEC;
		else if (noti->vbus_state.type == TCP_VBUS_CTRL_PD)
			pd_sink_type = SINK_TYPE_PD_TRY;
		else if (noti->vbus_state.type == TCP_VBUS_CTRL_REQUEST)
			pd_sink_type = SINK_TYPE_REQUEST;
		pr_info("%s sink vbus %dmv %dma type(%d)\n", __func__,
			pd_sink_voltage_new, pd_sink_current_new, pd_sink_type);
		mutex_unlock(&param_lock);

		if ((pd_sink_voltage_new != pd_sink_voltage_old) ||
		    (pd_sink_current_new != pd_sink_current_old)) {
			if (pd_sink_voltage_new) {
				/* enable charger */
#if CONFIG_MTK_GAUGE_VERSION == 30
				charger_manager_enable_power_path(chg_consumer,
							MAIN_CHARGER, true);
#else
				mtk_chr_pd_enable_power_path(1);
#endif
				pd_sink_voltage_old = pd_sink_voltage_new;
				pd_sink_current_old = pd_sink_current_new;
			} else if (pd_sink_type == SINK_TYPE_REMOVE) {
				if (tcpc_kpoc)
					break;
#if CONFIG_MTK_GAUGE_VERSION == 30
				charger_manager_enable_power_path(chg_consumer,
							MAIN_CHARGER, false);
#else
				mtk_chr_pd_enable_power_path(0);
#endif
				pd_sink_voltage_old = pd_sink_voltage_new;
				pd_sink_current_old = pd_sink_current_new;
			} else {
				bc12_chr_type = mt_get_charger_type();
				if (bc12_chr_type >= STANDARD_HOST &&
				    bc12_chr_type <= STANDARD_CHARGER)
					break;
				if (tcpc_kpoc)
					break;
				/* disable charge */
#if CONFIG_MTK_GAUGE_VERSION == 30
				charger_manager_enable_power_path(chg_consumer,
							MAIN_CHARGER, false);
#else
				mtk_chr_pd_enable_power_path(0);
#endif
				pd_sink_voltage_old = pd_sink_voltage_new;
				pd_sink_current_old = pd_sink_current_new;
			}
		}
		break;
	case TCP_NOTIFY_TYPEC_STATE:
		if (noti->typec_state.new_state == TYPEC_ATTACHED_SNK ||
		    noti->typec_state.new_state == TYPEC_ATTACHED_CUSTOM_SRC) {
			mutex_lock(&tcpc_usb_connect_lock);
#if CONFIG_MTK_GAUGE_VERSION == 30
		charger_dev_enable_chg_type_det(primary_charger, true);
#else
		mtk_chr_enable_chr_type_det(true);
#endif
			tcpc_usb_connected = true;
			mutex_unlock(&tcpc_usb_connect_lock);
			pr_info("%s USB Plug in, pol = %d\n", __func__,
				noti->typec_state.polarity);
#ifdef CONFIG_USB_C_SWITCH_U3_MUX
			if (noti->typec_state.polarity == 0)
				usb3_switch_ctrl_sel(CC1_SIDE);
			else
				usb3_switch_ctrl_sel(CC2_SIDE);
#endif
			if (!tcpm_get_boot_check_flag(tcpc_dev))
				tcpm_set_boot_check_flag(tcpc_dev, 1);
		} else if ((noti->typec_state.old_state == TYPEC_ATTACHED_SNK ||
			    noti->typec_state.old_state ==
					TYPEC_ATTACHED_CUSTOM_SRC) &&
			   noti->typec_state.new_state == TYPEC_UNATTACHED) {
			mutex_lock(&tcpc_usb_connect_lock);
			tcpc_usb_connected = false;
			mutex_unlock(&tcpc_usb_connect_lock);
			if (tcpc_kpoc) {
				vbus = battery_get_vbus();
				pr_info("%s KPOC Plug out, vbus = %d\n",
					__func__, vbus);
				tcpc_mt_power_off();
				break;
			}
#ifdef CONFIG_USB_PD_ALT_MODE_RTDC
			mutex_lock(&tcpc_pe30_rdy_lock);
			is_pep30_en_unlock = false;
			mutex_unlock(&tcpc_pe30_rdy_lock);
#endif /* CONFIG_USB_PD_ALT_MODE_RTDC */
#ifdef CONFIG_RT7207_ADAPTER
			tcpm_reset_pe30_ta(tcpc_dev);
#endif /* CONFIG_RT7207_ADAPTER */
			pr_info("%s USB Plug out\n", __func__);
#if CONFIG_MTK_GAUGE_VERSION == 30
			ret = charger_dev_enable_chg_type_det(primary_charger,
								false);
#else
			ret = mtk_chr_enable_chr_type_det(false);
#endif

#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
			boot_mode = get_boot_mode();
			if (ret < 0) {
				if (boot_mode ==
					KERNEL_POWER_OFF_CHARGING_BOOT ||
				    boot_mode == LOW_POWER_OFF_CHARGING_BOOT) {
					pr_info("%s: notify chg detach fail\n",
						__func__);
					mt_power_off();
				}
			}
#endif /* CONFIG_MTK_KERNEL_POWER_OFF_CHARGING */
		}
		break;
	case TCP_NOTIFY_PD_STATE:
		pr_info("%s pd state = %d\n",
			__func__, noti->pd_state.connected);
		mutex_lock(&tcpc_pd_rdy_lock);
		pd_state = noti->pd_state.connected;
		mutex_unlock(&tcpc_pd_rdy_lock);

#ifdef CONFIG_MTK_SMART_BATTERY
		pr_info("%s pd state = %d %d\n", __func__,
			noti->pd_state.connected, mtk_check_pe_ready_snk());
#if CONFIG_MTK_GAUGE_VERSION == 20
		if (mtk_is_pd_chg_ready() == true ||
		    mtk_is_pep30_en_unlock() == true)
			wake_up_bat3();
#endif
#endif /* CONFIG_MTK_SMART_BATTERY */
		break;
#ifdef CONFIG_USB_PD_ALT_MODE_RTDC
	case TCP_NOTIFY_DC_EN_UNLOCK:
		mutex_lock(&tcpc_pe30_rdy_lock);
		is_pep30_en_unlock = true;
		mutex_unlock(&tcpc_pe30_rdy_lock);
#ifdef CONFIG_MTK_SMART_BATTERY
#if CONFIG_MTK_GAUGE_VERSION == 20
		wake_up_bat3();
#endif
#endif /* CONFIG_MTK_SMART_BATTERY */

		pr_info("%s Direct Charge En Unlock\n", __func__);
		break;
#endif /* CONFIG_USB_PD_ALT_MODE_RTDC */
	case TCP_NOTIFY_EXIT_MODE:
		pr_info("%s Must stop PE3.0 right now\n", __func__);
		mutex_lock(&tcpc_pe30_rdy_lock);
		is_pep30_en_unlock = false;
		mutex_unlock(&tcpc_pe30_rdy_lock);
		break;
#ifdef CONFIG_TYPEC_NOTIFY_ATTACHWAIT_SNK
	case TCP_NOTIFY_ATTACHWAIT_SNK:
		pr_info("%s attach wait sink\n", __func__);
		break;
#endif /* CONFIG_TYPEC_NOTIFY_ATTACHWAIT_SNK */
	case TCP_NOTIFY_EXT_DISCHARGE:
		pr_info("%s ext discharge = %d\n", __func__, noti->en_state.en);
#if CONFIG_MTK_GAUGE_VERSION == 30
		charger_dev_enable_discharge(primary_charger,
			noti->en_state.en);
#endif
		break;

	case TCP_NOTIFY_HARD_RESET_STATE:
		if (noti->hreset_state.state == TCP_HRESET_RESULT_DONE ||
			noti->hreset_state.state == TCP_HRESET_RESULT_FAIL)
			charger_manager_enable_kpoc_shutdown(chg_consumer,
								true);
		else if (noti->hreset_state.state == TCP_HRESET_SIGNAL_SEND ||
			noti->hreset_state.state == TCP_HRESET_SIGNAL_RECV)
			charger_manager_enable_kpoc_shutdown(chg_consumer,
								false);
		break;
	default:
		break;
	};
	return NOTIFY_OK;
}

static int rt_pd_manager_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *node = pdev->dev.of_node;

	pr_info("%s (%s)\n", __func__, RT_PD_MANAGER_VERSION);
	if (node == NULL) {
		pr_info("%s devicd of node not exist\n", __func__);
		return -ENODEV;
	}

#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
	ret = get_boot_mode();
	if (ret == KERNEL_POWER_OFF_CHARGING_BOOT ||
	    ret == LOW_POWER_OFF_CHARGING_BOOT)
		tcpc_kpoc = true;
#endif /* CONFIG_MTK_KERNEL_POWER_OFF_CHARGING */
	pr_info("%s KPOC(%d)\n", __func__, tcpc_kpoc);

	/* Get charger device */
#if CONFIG_MTK_GAUGE_VERSION == 30
	primary_charger = get_charger_by_name("primary_chg");
	if (!primary_charger) {
		pr_info("%s: get primary charger device failed\n", __func__);
		return -ENODEV;
	}
	chg_consumer = charger_manager_get_by_name(&pdev->dev, "charger_port1");
	if (!chg_consumer) {
		pr_info("%s: get charger consumer device failed\n", __func__);
		return -ENODEV;
	}
#endif /* CONFIG_MTK_GAUGE_VERSION == 30 */

	tcpc_dev = tcpc_dev_get_by_name("type_c_port0");
	if (!tcpc_dev) {
		pr_info("%s get tcpc device type_c_port0 fail\n", __func__);
		return -ENODEV;
	}

	pd_nb.notifier_call = pd_tcp_notifier_call;
	ret = register_tcp_dev_notifier(tcpc_dev, &pd_nb);
	if (ret < 0) {
		pr_info("%s: register tcpc notifer fail\n", __func__);
		return -EINVAL;
	}

#ifdef CONFIG_RT7207_ADAPTER
	ret = mtk_direct_charge_vdm_init();
	if (ret < 0) {
		pr_info("%s mtk direct charge vdm init fail\n", __func__);
		return -EINVAL;
	}
#endif /* CONFIG_RT7207_ADAPTER */

	pr_info("%s OK!!\n", __func__);
	return ret;
}

static int rt_pd_manager_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id rt_pd_manager_of_match[] = {
	{ .compatible = "mediatek,rt-pd-manager" },
	{ }
};
MODULE_DEVICE_TABLE(of, rt_pd_manager_of_match);

static struct platform_driver rt_pd_manager_driver = {
	.driver = {
		.name = "rt-pd-manager",
		.of_match_table = of_match_ptr(rt_pd_manager_of_match),
	},
	.probe = rt_pd_manager_probe,
	.remove = rt_pd_manager_remove,
};

static int __init rt_pd_manager_init(void)
{
	return platform_driver_register(&rt_pd_manager_driver);
}

static void __exit rt_pd_manager_exit(void)
{
	platform_driver_unregister(&rt_pd_manager_driver);
}

late_initcall_sync(rt_pd_manager_init);
module_exit(rt_pd_manager_exit);

MODULE_AUTHOR("Jeff Chang");
MODULE_DESCRIPTION("Richtek pd manager driver");
MODULE_LICENSE("GPL");
