/*
 * Copyright (C) 2019 Motorola Mobility, Inc.
 *
 * Adding this driver is for moto chg tcmd's kernel interface.
 * By this driver, all chg tcmd's kernel file point would be just
 * in the folder of
 * /sys/devices/platform/moto_chg_tcmd/
 * So after then, tcmd app do not  need to change file access folder's name.
 * It will more convinient for all MTK projects with different charger IC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/power_supply.h>
#include <linux/power/moto_chg_tcmd.h>
#ifdef USE_LIST_HEAD
#include <linux/list.h>
#endif

#define DRIVERNAME	"moto_chg_tcmd"

struct moto_chg_tcmd_data {
	struct platform_device *pdev;
	struct power_supply *bat_psy;

	bool	factory_mode;
	const char* bat_psy_name;
	unsigned char reg_addr;
	unsigned char reg_val;

	int force_usb_suspend_flag;
	int pre_usb_current;
	int usb_current;

	int force_chg_enable_flag;
	int pre_chg_current;
	int chg_current;

	int usb_voltage;

	int bat_temp;
	int bat_voltage;
	int bat_ocv;
	int bat_id;
};

/*Struct pointer's work efficiency may be bester than list head*/
/*So if we just have battery and charger clients, we use struct pointer firstly*/
/*If there are much more clients later, we would use list head*/
#ifdef USE_LIST_HEAD
static LIST_HEAD(client_list);
#else
static struct moto_chg_tcmd_client* chg_client;
static struct moto_chg_tcmd_client* bat_client;
#endif

static ssize_t moto_chg_tcmd_addr_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%x\n", data->reg_addr);
}

static ssize_t moto_chg_tcmd_addr_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct  moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	int ret;
	unsigned char val;

	ret = kstrtou8(buf, 16, &val);
	if (ret) {
		pr_info("%s, %s not a valide buf(%d)\n", __func__, buf, ret);
		goto end;
	}

	data->reg_addr = val;

end:
	return count;
}
static DEVICE_ATTR(address, S_IWUSR | S_IRUGO, moto_chg_tcmd_addr_show, moto_chg_tcmd_addr_store);

static ssize_t moto_chg_tcmd_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	unsigned char val;
	int ret;

	if (!chg_client) {
		pr_err("%s chg_client is null\n", __func__);
		goto end;
	}

	ret = chg_client->reg_read(chg_client->data,
					data->reg_addr,
					&val);
	if (ret) {
		pr_err("%s read reg 0x%x fail %d\n", __func__,
			data->reg_addr, ret);
		data->reg_val = ret;
		goto end;
	}

	data->reg_val = val;

end:
	return snprintf(buf, PAGE_SIZE, "%x\n", data->reg_val);
}

static ssize_t moto_chg_tcmd_data_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct  moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	int ret;
	unsigned char val;

	if (!chg_client) {
		pr_err("%s chg_client is null\n", __func__);
		goto end;
	}

	ret = kstrtou8(buf, 16, &val);
	if (ret) {
		pr_info("%s, %s not a valide buf(%d)\n", __func__, buf, ret);
		goto end;
	}

	ret = chg_client->reg_write(chg_client->data,
					data->reg_addr,
					val);
	if (ret) {
		pr_err("%s write reg 0x%x = 0x%x fail %d\n", __func__,
			data->reg_addr, val, ret);
		goto end;
	}

end:
	return count;
}
static DEVICE_ATTR(data, S_IWUSR | S_IRUGO, moto_chg_tcmd_data_show, moto_chg_tcmd_data_store);

static ssize_t force_usb_suspend_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->force_usb_suspend_flag);
}

static ssize_t force_usb_suspend_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct  moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	int ret;
	int val;
	int cur;

	if (!chg_client) {
		pr_err("%s chg_client is null\n", __func__);
		goto end;
	}

	ret = kstrtouint(buf, 10, &val);
	if (ret) {
		pr_info("%s, %s not a valide buf(%d)\n", __func__, buf, ret);
		goto end;
	}

	val = !val;
	/*If charger driver has usb enable interface*/
	if (chg_client->set_usb_enable) {
		ret = chg_client->set_usb_enable(chg_client->data, val);
		if (ret) {
			pr_err("%s set usb en fail %d\n", __func__, ret);
			goto end;
		}

		data->force_usb_suspend_flag = val;
		goto end;
	}

	/*Or if charger driver do not have usb enable interface*/
	if (val) {
		ret = chg_client->get_usb_current(chg_client->data,
						&cur);
		if (ret) {
			pr_err("%s get usb cur fail %d\n", __func__, ret);
			goto end;
		}
		data->pre_usb_current = val;
	}

	ret = chg_client->set_usb_current(chg_client->data,
					val ? 0 : data->pre_usb_current);
	if (ret) {
		pr_err("%s set usb cur fail %d\n", __func__, ret);
		goto end;
	}

	data->force_usb_suspend_flag = val;
	data->usb_current = val ? 0 : data->pre_usb_current;

end:
	return count;
}
static DEVICE_ATTR(force_chg_usb_suspend, S_IWUSR | S_IRUGO,
	force_usb_suspend_show,force_usb_suspend_store);

static ssize_t force_chg_auto_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->force_chg_enable_flag);
}

static ssize_t force_chg_auto_enable_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct  moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	int ret;
	int val;
	int cur;

	if (!chg_client) {
		pr_err("%s chg_client is null\n", __func__);
		goto end;
	}

	ret = kstrtouint(buf, 10, &val);
	if (ret) {
		pr_info("%s, %s not a valide buf(%d)\n", __func__, buf, ret);
		goto end;
	}

	val = !!val;
	/*If charger driver has chg enable interface*/
	if (chg_client->set_chg_enable) {
		ret = chg_client->set_chg_enable(chg_client->data, val);
		if (ret) {
			pr_err("%s set chg en fail %d\n", __func__, ret);
			goto end;
		}

		data->force_chg_enable_flag = val;
		goto end;
	}

	/*Or if charger driver do not have chg enable interface*/
	if (!val) {
		ret = chg_client->get_chg_current(chg_client->data,
						&cur);
		if (ret) {
			pr_err("%s get chg cur fail %d\n", __func__, ret);
			goto end;
		}
		data->pre_chg_current = val;
	}

	ret = chg_client->set_chg_current(chg_client->data,
					val ? data->pre_chg_current : 0);
	if (ret) {
		pr_err("%s set chg cur fail %d\n", __func__, ret);
		goto end;
	}

	data->force_chg_enable_flag = val;
	data->chg_current = val ? data->pre_chg_current : 0;

end:
	return count;
}
static DEVICE_ATTR(force_chg_auto_enable, S_IWUSR | S_IRUGO,
	force_chg_auto_enable_show, force_chg_auto_enable_store);

static ssize_t force_chg_iusb_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->usb_current);
}

static ssize_t force_chg_iusb_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct  moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	int ret;
	int val;

	if (!chg_client) {
		pr_err("%s chg_client is null\n", __func__);
		goto end;
	}

	ret = kstrtoint(buf, 10, &val);
	if (ret) {
		pr_info("%s, %s not a valide buf(%d)\n", __func__, buf, ret);
		goto end;
	}

	ret = chg_client->set_usb_current(chg_client->data,
					val);
	if (ret) {
		pr_err("%s set usb cur %d fail %d\n", __func__, val, ret);
		goto end;
	}

	data->usb_current = val;

end:
	return count;
}
static DEVICE_ATTR(force_chg_iusb, S_IWUSR | S_IRUGO,
	force_chg_iusb_show, force_chg_iusb_store);

static ssize_t force_chg_ibatt_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->chg_current);
}

static ssize_t force_chg_ibatt_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct  moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	int ret;
	int val;

	if (!chg_client) {
		pr_err("%s chg_client is null\n", __func__);
		goto end;
	}

	ret = kstrtoint(buf, 10, &val);
	if (ret) {
		pr_info("%s, %s not a valide buf(%d)\n", __func__, buf, ret);
		goto end;
	}

	ret = chg_client->set_chg_current(chg_client->data,
					val);
	if (ret) {
		pr_err("%s set usb cur %d fail %d\n", __func__, val, ret);
		goto end;
	}

	data->chg_current = val;

end:
	return count;
}
static DEVICE_ATTR(force_chg_ibatt, S_IWUSR | S_IRUGO,
	force_chg_ibatt_show,force_chg_ibatt_store);


static ssize_t force_chg_fail_clear_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", "not valid");
}

static ssize_t force_chg_fail_clear_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	int ret;
	int val;

	if (!chg_client) {
		pr_err("%s chg_client is null\n", __func__);
		goto end;
	}

	ret = kstrtoint(buf, 10, &val);
	if (ret) {
		pr_info("%s, %s not a valide buf(%d)\n", __func__, buf, ret);
		goto end;
	}

	pr_info("%s just a dummy func\n", __func__);
end:
	return count;
}
static DEVICE_ATTR(force_chg_fail_clear, S_IWUSR | S_IRUGO,
	force_chg_fail_clear_show,force_chg_fail_clear_store);

static ssize_t usb_voltage_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	int val;
	int ret;

	if (!chg_client) {
		pr_err("%s chg_client is null\n", __func__);
		goto end;
	}

	ret = chg_client->get_usb_voltage(chg_client->data,
					&val);
	if (ret) {
		pr_err("%s get usb vol fail %d\n", __func__, ret);
		val = -EINVAL;
		goto end;
	}

	data->usb_voltage = val;

end:
	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t usb_voltage_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	pr_info("%s un-supported\n", __func__);
	return count;
}
static DEVICE_ATTR(usb_voltage, S_IWUSR | S_IRUGO,
	usb_voltage_show, usb_voltage_store);

static ssize_t bat_temp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	int val;
	int ret;

	if (!bat_client) {
		pr_err("%s bat_client is null\n", __func__);
		goto end;
	}

	ret = bat_client->get_bat_temp(bat_client->data,
					&val);
	if (ret) {
		pr_err("%s get bat temp fail %d\n", __func__, ret);
		val = -EINVAL;
		goto end;
	}

end:
	data->bat_temp = val;

	return snprintf(buf, PAGE_SIZE, "%d\n", data->bat_temp);
}

static ssize_t bat_temp_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	pr_info("%s un-supported\n", __func__);
	return count;
}
static DEVICE_ATTR(bat_temp, S_IWUSR | S_IRUGO,
	bat_temp_show, bat_temp_store);

static ssize_t bat_voltage_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	int val;
	int ret;

	if (!bat_client) {
		pr_err("%s bat_client is null\n", __func__);
		goto end;
	}

	ret = bat_client->get_bat_voltage(bat_client->data,
					&val);
	if (ret) {
		pr_err("%s get bat vol fail %d\n", __func__, ret);
		val = -EINVAL;
		goto end;
	}

end:
	data->bat_voltage = val;

	return snprintf(buf, PAGE_SIZE, "%d\n", data->bat_voltage);
}

static ssize_t bat_voltage_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	pr_info("%s un-supported\n", __func__);
	return count;
}
static DEVICE_ATTR(bat_voltage, S_IWUSR | S_IRUGO,
	bat_voltage_show, bat_voltage_store);

static ssize_t bat_ocv_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	int val;
	int ret;

	//MMI_STOPSHIP just return battery voltage firstly.
	if (!bat_client) {
		pr_err("%s bat client  is null\n", __func__);
		goto end;
	}

	ret = bat_client->get_bat_voltage(bat_client->data,
					&val);
	if (ret) {
		pr_err("%s get bat vol fail %d\n", __func__, ret);
		val = ret;
		goto end;
	}

end:
	data->bat_ocv = val;

	return snprintf(buf, PAGE_SIZE, "%d\n", data->bat_ocv);
}

static ssize_t bat_ocv_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	pr_info("%s un-supported\n", __func__);
	return count;
}
static DEVICE_ATTR(bat_ocv, S_IWUSR | S_IRUGO,
	bat_ocv_show, bat_ocv_store);

static ssize_t bat_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	int val;
	int ret;

	if (!bat_client) {
		pr_err("%s bat_client is null\n", __func__);
		goto end;
	}

	ret = bat_client->get_bat_id(bat_client->data,
					&val);
	if (ret) {
		pr_err("%s get bat id fail %d\n", __func__, ret);
		val = -EINVAL;
		goto end;
	}

end:
	data->bat_voltage = val;

	return snprintf(buf, PAGE_SIZE, "%d\n", data->bat_voltage);
}

static ssize_t bat_id_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	pr_info("%s un-supported\n", __func__);
	return count;
}
static DEVICE_ATTR(bat_id, S_IWUSR | S_IRUGO,
	bat_id_show, bat_id_store);

static struct attribute *moto_chg_tcmd_attrs[] = {
	&dev_attr_address.attr,
	&dev_attr_data.attr,
	&dev_attr_force_chg_usb_suspend.attr,
	&dev_attr_force_chg_auto_enable.attr,
	&dev_attr_force_chg_iusb.attr,
	&dev_attr_force_chg_ibatt.attr,
	&dev_attr_force_chg_fail_clear.attr,
	&dev_attr_usb_voltage.attr,
	&dev_attr_bat_temp.attr,
	&dev_attr_bat_voltage.attr,
	&dev_attr_bat_ocv.attr,
	&dev_attr_bat_id.attr,
	NULL,
};

ATTRIBUTE_GROUPS(moto_chg_tcmd);

int moto_chg_tcmd_get_bat_psy_prop(struct moto_chg_tcmd_data *data,
										int prop,
										int* value)
{
	union power_supply_propval val = {0, };
	int ret;

	if (!data->bat_psy) {
		data->bat_psy = power_supply_get_by_name(data->bat_psy_name);
		if (!data->bat_psy) {
			pr_err("%s get bat psy error\n", __func__);
			ret = -ENODEV;
			goto end;
		}
	}

	ret = power_supply_get_property(data->bat_psy, prop, &val);
	if (ret) {
		pr_err("battery psy reading Prop %d rc = %d\n", prop, ret);
		ret = -EINVAL;
		goto end;
	}

	*value = val.intval;

end:
	return ret;
}

int moto_chg_tcmd_register(struct moto_chg_tcmd_client *client)
{
#ifdef USE_LIST_HEAD
	struct moto_chg_tcmd_client *existing;
#endif

	if (!client) {
		pr_err("%s invalide chg tcmd client(null)\n", __func__);
		return -EINVAL;
	}

#ifdef USE_LIST_HEAD
	list_add_tail(&client->list, &head_list);
#else
	switch (client->client_id) {
		case MOTO_CHG_TCMD_CLIENT_CHG:
			chg_client = client;
			break;
		case MOTO_CHG_TCMD_CLIENT_BAT:
			bat_client = client;
			break;
		default :
			pr_err("%s invalide client id %d\n",
				__func__,
				client->client_id);
			break;
	}
#endif

	return 0;
}

static int moto_chg_tcmd_parse_dt(struct moto_chg_tcmd_data *data)
{
	struct device_node *node = data->pdev->dev.of_node;
	int ret = 0;

	if (!node) {
		pr_err("%s dtree info. missing\n", __func__);
		return -ENODEV;
	}

	ret = of_property_read_string(node, "mmi,bat-supply-name",
				     &data->bat_psy_name);
	if (ret) {
		pr_info("%s do not define battery psy name, used default\n", __func__);
		data->bat_psy_name = "battery";
	}

	return 0;
}

static int moto_chg_tcmd_probe(struct platform_device *pdev)
{
	struct moto_chg_tcmd_data *data;
	int ret;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->pdev = pdev;
	platform_set_drvdata(pdev, data);

	ret = sysfs_create_groups(&pdev->dev.kobj, moto_chg_tcmd_groups);
	if (ret) {
		dev_err(&pdev->dev, "%s Failed to create sysfs attributes\n", __func__);
		return ret;
	}

	moto_chg_tcmd_parse_dt(data);

	data->bat_psy = power_supply_get_by_name(data->bat_psy_name);

	dev_info(&pdev->dev, "%s probe success\n", __func__);

	return 0;
}

static int moto_chg_tcmd_remove(struct platform_device *pdev)
{
	sysfs_remove_groups(&pdev->dev.kobj, moto_chg_tcmd_groups);
	//kobject_uevent(&pdev->dev.kobj, KOBJ_REMOVE);
	platform_set_drvdata(pdev, NULL);

	dev_info(&pdev->dev, "%s remove success\n", __func__);

	return 0;
}


static struct of_device_id  moto_chg_tcmd_match_table[] = {
	{ .compatible = "mmi,moto-chg-tcmd"},
	{}
};


static struct platform_driver moto_chg_tcmd_driver = {
	.probe	= moto_chg_tcmd_probe,
	.remove = moto_chg_tcmd_remove,
	.driver = {
		.name = DRIVERNAME,
		.of_match_table =  moto_chg_tcmd_match_table,
	},
};

int __init moto_chg_tcmd_init(void)
{
	int ret;

	ret = platform_driver_register(&moto_chg_tcmd_driver);

	return ret;
}

void moto_chg_tcmd_exit(void)
{
	platform_driver_unregister(&moto_chg_tcmd_driver);
}

module_init(moto_chg_tcmd_init);
module_exit(moto_chg_tcmd_exit);

MODULE_DESCRIPTION("moto_chg_tcmd Driver");
MODULE_AUTHOR("Motorola");
MODULE_LICENSE("GPL");
