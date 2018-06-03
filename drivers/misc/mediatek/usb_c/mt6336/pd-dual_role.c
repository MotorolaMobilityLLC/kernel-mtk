/*
 * Copyright (C) 2016 MediaTek Inc.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include "mtk_typec.h"

#ifdef CONFIG_DUAL_ROLE_USB_INTF
static enum dual_role_property mt_dual_role_props[] = {
	DUAL_ROLE_PROP_MODE,
	DUAL_ROLE_PROP_PR,
	DUAL_ROLE_PROP_DR,
	DUAL_ROLE_PROP_VCONN_SUPPLY,
};

static int mt_dual_role_get_prop(struct dual_role_phy_instance *dual_role,
			enum dual_role_property prop, unsigned int *val)
{
	struct typec_hba *hba = dev_get_drvdata(dual_role->dev.parent);
	int ret = 0;

	switch (prop) {
	case DUAL_ROLE_PROP_MODE:
		*val = hba->dual_role_mode;
		break;
	case DUAL_ROLE_PROP_PR:
		*val = hba->dual_role_pr;
		break;
	case DUAL_ROLE_PROP_DR:
		*val = hba->dual_role_dr;
		break;
	case DUAL_ROLE_PROP_VCONN_SUPPLY:
		*val = hba->dual_role_vconn;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int mt_dual_role_prop_is_writeable(
	struct dual_role_phy_instance *dual_role, enum dual_role_property prop)
{
	int retval = -EROFS;
	struct typec_hba *hba = dev_get_drvdata(dual_role->dev.parent);

	switch (prop) {
#if SUPPORT_PD
	case DUAL_ROLE_PROP_PR:
	case DUAL_ROLE_PROP_DR:
	case DUAL_ROLE_PROP_VCONN_SUPPLY:
		if (hba->dual_role_supported_modes ==
			DUAL_ROLE_SUPPORTED_MODES_DFP_AND_UFP)
			retval = 1;
		break;
#endif
	default:
		break;
	}
	return retval;
}

static int mt_dual_role_set_prop(struct dual_role_phy_instance *dual_role,
			enum dual_role_property prop, const unsigned int *val)
{
	struct typec_hba *hba = dev_get_drvdata(dual_role->dev.parent);

	switch (prop) {
	case DUAL_ROLE_PROP_PR:
		if (*val != hba->dual_role_pr) {
			pr_info("%s power role swap (%d->%d)\n",
				__func__, hba->dual_role_pr, *val);
			pd_request_power_swap(hba);

			/*Tigger pd_task to do DR_SWAP*/
			hba->rx_event = true;
			wake_up(&hba->wq);
		} else
			pr_info("%s Same Power Role\n", __func__);
		break;
	case DUAL_ROLE_PROP_DR:
		if (*val != hba->dual_role_dr) {
			pr_info("%s data role swap (%d->%d)\n",
				__func__, hba->dual_role_dr, *val);
			pd_request_data_swap(hba);

			/*Tigger pd_task to do DR_SWAP*/
			hba->rx_event = true;
			wake_up(&hba->wq);
		} else
			pr_info("%s Same Data Role\n", __func__);
		break;
	case DUAL_ROLE_PROP_VCONN_SUPPLY:
		if (*val != hba->dual_role_vconn) {
			pr_info("%s vconn swap (%d->%d)\n",
				__func__, hba->dual_role_vconn, *val);
			pd_request_vconn_swap(hba);

			/*Tigger pd_task to do DR_SWAP*/
			hba->rx_event = true;
			wake_up(&hba->wq);
		} else
			pr_info("%s Same Vconn\n", __func__);
		break;
	default:
		break;
	}
	return 0;
}

int mt_dual_role_phy_init(struct typec_hba *hba)
{
	struct dual_role_phy_desc *dual_desc;
	int len;
	char *str_name;

	hba->dr_usb = devm_kzalloc(hba->dev,
				sizeof(hba->dr_usb), GFP_KERNEL);

	dual_desc = devm_kzalloc(hba->dev, sizeof(*dual_desc), GFP_KERNEL);
	if (!dual_desc)
		return -ENOMEM;

	if (hba->support_role == TYPEC_ROLE_DRP)
		dual_desc->supported_modes = DUAL_ROLE_SUPPORTED_MODES_DFP_AND_UFP;
	else
		dual_desc->supported_modes = DUAL_ROLE_SUPPORTED_MODES_UFP;

	len = strlen(dev_driver_string(hba->dev));
	str_name = devm_kzalloc(hba->dev, len+11, GFP_KERNEL);
	sprintf(str_name, "dual-role-%s", dev_driver_string(hba->dev));
	dual_desc->name = str_name;

	dual_desc->properties = mt_dual_role_props;
	dual_desc->num_properties = ARRAY_SIZE(mt_dual_role_props);
	dual_desc->get_property = mt_dual_role_get_prop;
	dual_desc->set_property = mt_dual_role_set_prop;
	dual_desc->property_is_writeable = mt_dual_role_prop_is_writeable;

	hba->dr_usb = devm_dual_role_instance_register(hba->dev, dual_desc);
	if (IS_ERR(hba->dr_usb)) {
		dev_err(hba->dev, "fail to register dual role usb\n");
		return -EINVAL;
	}

	/* init dual role phy instance property */
	hba->dual_role_pr = DUAL_ROLE_PROP_PR_NONE;
	hba->dual_role_dr = DUAL_ROLE_PROP_DR_NONE;
	hba->dual_role_mode = DUAL_ROLE_PROP_MODE_NONE;
	hba->dual_role_vconn = DUAL_ROLE_PROP_VCONN_SUPPLY_NO;
	return 0;
}
#endif /* CONFIG_DUAL_ROLE_USB_INTF */
