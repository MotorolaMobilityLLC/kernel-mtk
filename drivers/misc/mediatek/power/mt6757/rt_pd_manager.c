/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * drivers/misc/mediatek/power/mt6757/rt_pd_manager.c
 *
 * Author: Sakya <jeff_chang@richtek.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <mt-plat/pd/tcpm.h>
#include <linux/of_device.h>

#include <mt-plat/battery_common.h>
#include <mt-plat/pd/mtk_direct_charge_vdm.h>

enum {
	MTK_USB_UNATTACHE,
	MTK_USB_ATTACHED,
	MTK_OTG_ATTACHED,
};

static struct tcpc_device *tcpc_dev;
static struct notifier_block pd_nb;
static struct mutex param_lock;
static unsigned char pd_state;
static u8 pre_attache_status;
static u8 now_attache_status;
static int pd_sink_voltage;
static int pd_sink_current;
static int pd_source_voltage;
static int pd_source_current;


bool mtk_check_pe_ready_snk(void)
{
	bool pe_ready_snk;

	mutex_lock(&param_lock);
	pe_ready_snk = (pd_state == PD_CONNECT_PE_READY_SNK) ? true : false;
	mutex_unlock(&param_lock);
	return pe_ready_snk;
}

static int pd_tcp_notifier_call(struct notifier_block *nb,
					unsigned long event, void *data)
{
	struct tcp_notify *noti = data;

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
		mutex_lock(&param_lock);
		pr_info("%s source vbus %dmv %dma attach:%d\n", __func__,
			noti->vbus_state.mv, noti->vbus_state.ma,
			now_attache_status);
		pd_source_voltage = noti->vbus_state.mv;
		pd_source_current = noti->vbus_state.ma;
		mutex_unlock(&param_lock);
		break;
	case TCP_NOTIFY_SINK_VBUS:
		mutex_lock(&param_lock);
		pr_info("%s sink vbus %dmv %dma attach:%d\n", __func__,
			noti->vbus_state.mv, noti->vbus_state.ma,
			now_attache_status);
		pd_sink_voltage = noti->vbus_state.mv;
		pd_sink_current = noti->vbus_state.ma;
		mutex_unlock(&param_lock);
		break;
	case TCP_NOTIFY_TYPEC_STATE:
		if (noti->typec_state.new_state == TYPEC_ATTACHED_SNK) {
			pr_info("%s USB Plug in, pol = %d\n", __func__,
				noti->typec_state.polarity);
			mutex_lock(&param_lock);
			pre_attache_status = now_attache_status;
			now_attache_status = MTK_USB_ATTACHED;
			mt_usb_connect();
			mutex_unlock(&param_lock);
		} else if (noti->typec_state.old_state == TYPEC_ATTACHED_SNK &&
			noti->typec_state.new_state == TYPEC_UNATTACHED) {
			pr_info("%s USB Plug out\n", __func__);
			mutex_lock(&param_lock);
			pre_attache_status = now_attache_status;
			now_attache_status = MTK_USB_UNATTACHE;
			mt_usb_disconnect();
			mutex_unlock(&param_lock);
		} else if (noti->typec_state.new_state == TYPEC_ATTACHED_SRC) {
			pr_info("%s OTG Plug in\n", __func__);
			mutex_lock(&param_lock);
			pre_attache_status = now_attache_status;
			now_attache_status = MTK_OTG_ATTACHED;
			mutex_unlock(&param_lock);
		} else if (noti->typec_state.old_state == TYPEC_ATTACHED_SRC &&
			noti->typec_state.new_state == TYPEC_UNATTACHED) {
			pr_info("%s OTG Plug out\n", __func__);
			mutex_lock(&param_lock);
			pre_attache_status = now_attache_status;
			now_attache_status = MTK_USB_UNATTACHE;
			mutex_unlock(&param_lock);
		}
		break;
	case TCP_NOTIFY_PD_STATE:
		pr_info("%s pd state = %d\n",
			__func__, noti->pd_state.connected);
		mutex_lock(&param_lock);
		pd_state = noti->pd_state.connected;
		mutex_unlock(&param_lock);
		break;
	default:
		break;
	};
	return NOTIFY_OK;
}

static int rt_pd_manager_probe(struct platform_device *pdev)
{
	int ret = 0;

	pr_info("%s\n", __func__);
	mutex_init(&param_lock);
	tcpc_dev = tcpc_dev_get_by_name("type_c_port0");
	if (!tcpc_dev) {
		pr_err("%s get tcpc device type_c_port0 fail\n", __func__);
		return -ENODEV;
	}

	pd_nb.notifier_call = pd_tcp_notifier_call;
	ret = register_tcp_dev_notifier(tcpc_dev, &pd_nb);
	if (ret < 0) {
		pr_err("%s: register tcpc notifer fail\n", __func__);
		return -EINVAL;
	}

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
