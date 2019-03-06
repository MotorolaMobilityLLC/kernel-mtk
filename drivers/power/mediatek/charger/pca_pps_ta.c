/*
 * Copyright (C) 2019 MediaTek Inc.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/notifier.h>

#include <tcpm.h>
#include <mt-plat/prop_chgalgo_class.h>

#define PCA_PPS_TA_VERSION	"1.0.0_G"
#define PCA_PPS_CMD_RETRY_COUNT	2

struct pca_pps_info {
	struct device *dev;
	struct prop_chgalgo_device *pca;
	struct tcpc_device *tcpc;
	struct notifier_block tcp_nb;
	bool is_pps_en_unlock;
	int hrst_cnt;
};

static inline int check_typec_attached_snk(struct tcpc_device *tcpc)
{
	if (tcpm_inquire_typec_attach_state(tcpc) != TYPEC_ATTACHED_SNK)
		return -EINVAL;
	return 0;
}

static int pca_pps_enable_charging(struct prop_chgalgo_device *pca, bool en,
				   u32 mV, u32 mA)
{
	struct pca_pps_info *info = prop_chgalgo_get_drvdata(pca);
	int ret, cnt = 0;

	if (check_typec_attached_snk(info->tcpc) < 0)
		return -EINVAL;
	PCA_DBG("en = %d, %dmV, %dmA\n", en, mV, mA);

	do {
		if (en)
			ret = tcpm_set_apdo_charging_policy(info->tcpc,
				DPM_CHARGING_POLICY_PPS, mV, mA, NULL);
		else
			ret = tcpm_reset_pd_charging_policy(info->tcpc, NULL);
		cnt++;
	} while (ret != TCPM_SUCCESS && cnt < PCA_PPS_CMD_RETRY_COUNT);

	return ret;
}

static int pca_pps_set_cap(struct prop_chgalgo_device *pca, u32 mV, u32 mA)
{
	struct pca_pps_info *info = prop_chgalgo_get_drvdata(pca);
	int ret, cnt = 0;

	if (check_typec_attached_snk(info->tcpc) < 0)
		return -EINVAL;
	PCA_DBG("%dmV, %dmA\n", mV, mA);

	if (!tcpm_inquire_pd_connected(info->tcpc)) {
		PCA_ERR("pd not connected\n");
		return -EINVAL;
	}

	do {
		ret = tcpm_dpm_pd_request(info->tcpc, mV, mA, NULL);
		cnt++;
	} while (ret != TCPM_SUCCESS && cnt < PCA_PPS_CMD_RETRY_COUNT);

	return ret;
}


static int pca_pps_get_measure_cap(struct prop_chgalgo_device *pca, u32 *mV,
				   u32 *mA)
{
	struct pca_pps_info *info = prop_chgalgo_get_drvdata(pca);
	int ret, cnt = 0;
	struct pd_pps_status pps_status;

	if (check_typec_attached_snk(info->tcpc) < 0)
		return -EINVAL;

	do {
		ret = tcpm_dpm_pd_get_pps_status(info->tcpc, NULL, &pps_status);
		cnt++;
	} while (ret != TCPM_SUCCESS && cnt < PCA_PPS_CMD_RETRY_COUNT);

	if (ret == TCPM_SUCCESS) {
		*mV = pps_status.output_mv;
		*mA = pps_status.output_ma;
	}
	PCA_DBG("%dmV, %dmA, ret(%d)\n", *mV, *mA, ret);

	return ret;
}

static int pca_pps_get_temperature(struct prop_chgalgo_device *pca, int *temp)
{
	struct pca_pps_info *info = prop_chgalgo_get_drvdata(pca);
	int ret, cnt = 0;
	struct pd_status status;

	if (check_typec_attached_snk(info->tcpc) < 0)
		return -EINVAL;

	do {
		ret = tcpm_dpm_pd_get_status(info->tcpc, NULL, &status);
		cnt++;
	} while (ret != TCPM_SUCCESS && cnt < PCA_PPS_CMD_RETRY_COUNT);

	if (ret == TCPM_SUCCESS) {
		*temp = status.internal_temp;
		PCA_DBG("%d degree\n", *temp);
	}

	return ret;
}

static int pca_pps_get_status(struct prop_chgalgo_device *pca,
			      struct prop_chgalgo_ta_status *ta_status)
{
	struct pca_pps_info *info = prop_chgalgo_get_drvdata(pca);
	int ret, cnt = 0;
	struct pd_status status;

	if (check_typec_attached_snk(info->tcpc) < 0)
		return -EINVAL;

	do {
		ret = tcpm_dpm_pd_get_status(info->tcpc, NULL, &status);
		cnt++;
	} while (ret != TCPM_SUCCESS && cnt < PCA_PPS_CMD_RETRY_COUNT);

	if (ret == TCPM_SUCCESS) {
		ta_status->temp1 = status.internal_temp;
		ta_status->present_input = status.present_input;
		ta_status->present_battery_input = status.present_battey_input;
		ta_status->ocp = status.event_flags & PD_STASUS_EVENT_OCP;
		ta_status->otp = status.event_flags & PD_STATUS_EVENT_OTP;
		ta_status->ovp = status.event_flags & PD_STATUS_EVENT_OVP;
		ta_status->cc_mode =
			status.event_flags & PD_STATUS_EVENT_CF_MODE;
		ta_status->temp_level = PD_STATUS_TEMP_PTF(status.temp_status);
	}
	return ret;
}

static int pca_pps_send_hardreset(struct prop_chgalgo_device *pca)
{
	struct pca_pps_info *info = prop_chgalgo_get_drvdata(pca);
	int ret, cnt = 0;

	if (check_typec_attached_snk(info->tcpc) < 0)
		return -EINVAL;

	PCA_INFO("++\n");
	do {
		ret = tcpm_dpm_pd_hard_reset(info->tcpc, NULL);
		cnt++;
	} while (ret != TCPM_SUCCESS && cnt < PCA_PPS_CMD_RETRY_COUNT);

	return ret;
}

static int pca_pps_authenticate_ta(struct prop_chgalgo_device *pca,
				   struct prop_chgalgo_ta_auth_data *data)
{
	int ret, apdo_idx = -1;
	struct pca_pps_info *info = prop_chgalgo_get_drvdata(pca);
	struct tcpm_power_cap_val apdo_cap;
	u8 cap_idx;

	PCA_INFO("++\n");
	if (check_typec_attached_snk(info->tcpc) < 0)
		return -EINVAL;

	if (!info->is_pps_en_unlock) {
		PCA_ERR("pps en is locked\n");
		return -EINVAL;
	}

	if (!tcpm_inquire_pd_pe_ready(info->tcpc)) {
		PCA_ERR("PD PE not ready\n");
		return -EINVAL;
	}

	/* select TA boundary */
	cap_idx = 0;
	while (1) {
		ret = tcpm_inquire_pd_source_apdo(info->tcpc,
						  TCPM_POWER_CAP_APDO_TYPE_PPS,
						  &cap_idx, &apdo_cap);
		if (ret != TCPM_SUCCESS) {
			PCA_ERR("inquire pd apdo fail(%d)\n", ret);
			break;
		}

		PCA_INFO("cap_idx[%d], %d mv ~ %d mv, %d ma\n", cap_idx,
			 apdo_cap.min_mv, apdo_cap.max_mv, apdo_cap.ma);

		if (apdo_cap.min_mv <= data->cap_vmin &&
		    apdo_cap.max_mv >= data->cap_vmax &&
		    apdo_cap.ma >= data->cap_imin) {
			apdo_idx = cap_idx;
			data->ta_vmin = apdo_cap.min_mv;
			data->ta_vmax = apdo_cap.max_mv;
			data->ta_imax = apdo_cap.ma;
			break;
		}
	}

	if (apdo_idx == -1) {
		PCA_ERR("cannot find apdo for pps algo\n");
		return -EINVAL;
	}
	return 0;
}

static int pca_pps_enable_wdt(struct prop_chgalgo_device *pca, bool en)
{
	return 0;
}

static int pca_pps_set_wdt(struct prop_chgalgo_device *pca, u32 ms)
{
	return 0;
}

static struct prop_chgalgo_desc pca_pps_desc = {
	.name = "pca_ta_pps",
	.type = PCA_DEVTYPE_TA,
};

static struct prop_chgalgo_ta_ops pca_pps_ops = {
	.enable_charging = pca_pps_enable_charging,
	.set_cap = pca_pps_set_cap,
	.get_measure_cap = pca_pps_get_measure_cap,
	.get_temperature = pca_pps_get_temperature,
	.get_status = pca_pps_get_status,
	.send_hardreset = pca_pps_send_hardreset,
	.authenticate_ta = pca_pps_authenticate_ta,
	.enable_wdt = pca_pps_enable_wdt,
	.set_wdt = pca_pps_set_wdt,
};

static int pca_pps_tcp_notifier_call(struct notifier_block *nb,
				     unsigned long event, void *data)
{
	struct pca_pps_info *info =
		container_of(nb, struct pca_pps_info, tcp_nb);
	struct tcp_notify *noti = data;

	switch (event) {
	case TCP_NOTIFY_PD_STATE:
		switch (noti->pd_state.connected) {
		case PD_CONNECT_NONE:
			dev_info(info->dev, "detached\n");
			info->is_pps_en_unlock = false;
			info->hrst_cnt = 0;
			break;
		case PD_CONNECT_HARD_RESET:
			info->hrst_cnt++;
			dev_info(info->dev, "pd hardreset, cnt = %d\n",
				 info->hrst_cnt);
			info->is_pps_en_unlock = false;
			break;
		case PD_CONNECT_PE_READY_SNK_APDO:
			if (info->hrst_cnt < 5) {
				dev_info(info->dev, "%s en unlock\n", __func__);
				info->is_pps_en_unlock = true;
			}
			break;
		default:
			break;
		}
	default:
		break;
	}
	return NOTIFY_OK;
}

static int pca_pps_probe(struct platform_device *pdev)
{
	int ret;
	struct pca_pps_info *info;

	dev_info(&pdev->dev, "%s\n", __func__);

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;
	info->dev = &pdev->dev;
	platform_set_drvdata(pdev, info);

	info->tcpc = tcpc_dev_get_by_name("type_c_port0");
	if (!info->tcpc) {
		dev_info(info->dev, "%s get tcpc dev fail\n", __func__);
		return -ENODEV;
	}

	/* register tcp notifier callback */
	info->tcp_nb.notifier_call = pca_pps_tcp_notifier_call;
	ret = register_tcp_dev_notifier(info->tcpc, &info->tcp_nb,
					TCP_NOTIFY_TYPE_USB);
	if (ret < 0) {
		dev_info(info->dev, "register tcpc notifier fail\n");
		return ret;
	}

	info->pca = prop_chgalgo_device_register(info->dev, &pca_pps_desc,
						 &pca_pps_ops, NULL, NULL,
						 info);
	if (!info->pca) {
		dev_info(info->dev, "%s register pps fail\n", __func__);
		return -EINVAL;
	}

	dev_info(info->dev, "%s successfully\n", __func__);
	return 0;
}

static int pca_pps_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_device pca_pps_platdev = {
	.name = "pca_pps",
	.id = -1,
};

static struct platform_driver pca_pps_platdrv = {
	.probe = pca_pps_probe,
	.remove = pca_pps_remove,
	.driver = {
		.name = "pca_pps",
		.owner = THIS_MODULE,
	},
};

static int __init pca_pps_init(void)
{
	platform_device_register(&pca_pps_platdev);
	return platform_driver_register(&pca_pps_platdrv);
}
module_init(pca_pps_init);

static void __exit pca_pps_exit(void)
{
	platform_driver_unregister(&pca_pps_platdrv);
	platform_device_unregister(&pca_pps_platdev);
}
module_exit(pca_pps_exit);

MODULE_DESCRIPTION("Programmable Power Supply TA For PCA");
MODULE_AUTHOR("ShuFan Lee<shufan_lee@richtek.com>");
MODULE_VERSION(PCA_PPS_TA_VERSION);
MODULE_LICENSE("GPL");
