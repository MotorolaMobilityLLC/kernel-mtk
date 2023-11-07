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

/*-20 ~ 80 num is 2*10 + 1*/
#define NTC_TABLE_VALID_NUM	21

enum MOTO_ADC_TCMD_CHANNEL {
	MOTO_ADC_TCMD_CHANNEL_CPU = 0,
	MOTO_ADC_TCMD_CHANNEL_CHG,
	MOTO_ADC_TCMD_CHANNEL_PA,
	MOTO_ADC_TCMD_CHANNEL_BATID,
	MOTO_ADC_TCMD_CHANNEL_VBAT,
	MOTO_ADC_TCMD_CHANNEL_MAX
};

struct CHARGER_TEMPERATURE {
	__s32 CHARGER_Temp;
	__s32 TemperatureR;
};

struct moto_chg_tcmd_data {
	struct platform_device *pdev;
	struct power_supply *bat_psy;
	struct CHARGER_TEMPERATURE* ntc_table;

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
	int chg_type;
	int chg_watt;

	int usb_voltage;

	int bat_temp;
	int bat_voltage;
	int bat_ocv;
	int bat_id;

	int batid_v_ref;
	int batid_r_pull;
	int ntc_v_ref;
	int ntc_r_pull;
	int adc_channel_list[MOTO_ADC_TCMD_CHANNEL_MAX];
	int cur_adc_channel;
};

/*Struct pointer's work efficiency may be bester than list head*/
/*So if we just have battery and charger clients, we use struct pointer firstly*/
/*If there are much more clients later, we would use list head*/
#ifdef USE_LIST_HEAD
static LIST_HEAD(client_list);
#else
static struct moto_chg_tcmd_client* chg_client;
static struct moto_chg_tcmd_client* bat_client;
static struct moto_chg_tcmd_client* pm_adc_client;
static struct moto_chg_tcmd_client* ap_adc_client;
static struct moto_chg_tcmd_client* wls_client;
#endif

/* NCP15WF104F03RC(100K) */
static struct CHARGER_TEMPERATURE ntc_table1[] = {
	{-40, 4397119},
	{-35, 3088599},
	{-30, 2197225},
	{-25, 1581881},
	{-20, 1151037},
	{-15, 846579},
	{-10, 628988},
	{-5, 471632},
	{0, 357012},
	{5, 272500},
	{10, 209710},
	{15, 162651},
	{20, 127080},
	{25, 100000},		/* 100K */
	{30, 79222},
	{35, 63167},
#if defined(APPLY_PRECISE_NTC_TABLE)
	{40, 50677},
	{41, 48528},
	{42, 46482},
	{43, 44533},
	{44, 42675},
	{45, 40904},
	{46, 39213},
	{47, 37601},
	{48, 36063},
	{49, 34595},
	{50, 33195},
	{51, 31859},
	{52, 30584},
	{53, 29366},
	{54, 28203},
	{55, 27091},
	{56, 26028},
	{57, 25013},
	{58, 24042},
	{59, 23113},
	{60, 22224},
#else
	{40, 50677},
	{45, 40904},
	{50, 33195},
	{55, 27091},
	{60, 22224},
#endif
	{65, 18323},
	{70, 15184},
	{75, 12635},
	{80, 10566},
	{85, 8873},
	{90, 7481},
	{95, 6337},
	{100, 5384},
	{105, 4594},
	{110, 3934},
	{115, 3380},
	{120, 2916},
	{125, 2522}
};

static int moto_chg_tcmd_res_to_temp(struct moto_chg_tcmd_data *data,
					int vol, int vol_ntc, int r_pull);

static int moto_chg_get_adc_value(struct  moto_chg_tcmd_data *data,
				struct moto_chg_tcmd_client* client, int channel, int* val)
{
	int ret;

	if ((channel < 0) || (client==NULL)) {
		ret = channel;
		pr_err("%s adc client or channel %d is not defined\n", __func__, ret);
		goto end;
	}

	pr_info("%s read adc channel %d\n", __func__, channel);
	ret = client->get_adc_value(client->data,
							channel,
							val);
	if (ret) {
		*val = ret;
		pr_err("%s read adc %d error %d\n",
			__func__,
			channel,
			ret);
		goto end;
	}

end:
	return ret;
}

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
						&val);
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
	int ret;
	int val;

	val = data->chg_current;
	if (val <= 0) {
		ret = chg_client->get_chg_current(chg_client->data,
						&val);
		if (ret) {
			pr_err("%s get chg cur fail %d\n", __func__, ret);
			goto end;
		}
		data->chg_current = val;
	}

end:
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
	int val = 0;
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
	int val = 0;
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
	data->bat_temp = val;

end:
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
	int val = 0;
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
	data->bat_voltage = val;

end:
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
	int val = 0;
	int ret;

	if (!bat_client) {
		pr_err("%s bat client  is null\n", __func__);
		goto end;
	}

	ret = bat_client->get_bat_ocv(bat_client->data,
					&val);
	if (ret) {
		pr_err("%s get bat vol fail %d\n", __func__, ret);
		val = ret;
		goto end;
	}
	data->bat_ocv = val;

end:
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

static ssize_t bat_cycle_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int val = -1;
	int ret = 0;

	if (!bat_client) {
		pr_err("%s bat client  is null\n", __func__);
		goto end;
	}

	if (bat_client->get_bat_cycle) {
		pr_err("%s get_bat_cycle is null\n", __func__);
		goto end;
	}

	ret = bat_client->get_bat_cycle(bat_client->data, &val);
	if (ret) {
		pr_err("%s get bat cycle fail %d\n", __func__, ret);
		goto end;
	}

end:
	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t bat_cycle_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	int val = 0;
	int ret = 0;

	if (!bat_client) {
		pr_err("%s bat client is null\n", __func__);
		goto end;
	}
	ret = kstrtoint(buf, 10, &val);
	if (ret) {
		pr_info("%s, %s not a valide buf(%d)\n", __func__, buf, ret);
		goto end;
	}

	if (!bat_client->set_bat_cycle) {
		pr_err("%s set_bat_cycle is null\n", __func__);
		goto end;
	}

	ret = bat_client->set_bat_cycle(bat_client->data, val);
	if (ret) {
		pr_err("%s set_bat_cycle fail %d\n", __func__, ret);
		goto end;
	}

end:
	return count;
}
static DEVICE_ATTR(bat_cycle, S_IWUSR | S_IRUGO,
	bat_cycle_show, bat_cycle_store);


static ssize_t chg_type_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	int type;
	int ret;

	if (!chg_client) {
		pr_err("%s bat client  is null\n", __func__);
		goto end;
	}

	ret = chg_client->get_charger_type(chg_client->data,
					&type);
	if (ret) {
		pr_err("%s get chg type fail %d\n", __func__, ret);
		type = ret;
		goto end;
	}
	data->chg_type = type;

end:

	return snprintf(buf, PAGE_SIZE, "%02d\n", data->chg_type);
}

static ssize_t chg_type_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	pr_info("%s un-supported\n", __func__);
	return count;
}
static DEVICE_ATTR(chg_type, S_IWUSR | S_IRUGO,
	chg_type_show, chg_type_store);

static ssize_t chg_watt_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	int watt;
	int ret;

	if (!chg_client) {
		pr_err("%s bat client  is null\n", __func__);
		goto end;
	}

	ret = chg_client->get_charger_watt(chg_client->data,
					&watt);
	if (ret) {
		pr_err("%s get chg type fail %d\n", __func__, ret);
		watt = ret;
		goto end;
	}
	data->chg_watt = watt;

end:

	return snprintf(buf, PAGE_SIZE, "%02d\n", data->chg_watt);
}

static ssize_t chg_watt_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	pr_info("%s un-supported\n", __func__);
	return count;
}
static DEVICE_ATTR(chg_watt, S_IWUSR | S_IRUGO,
	chg_watt_show, chg_watt_store);

static ssize_t bat_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	int val;
	int ret;

	if (!bat_client) {
		pr_err("%s bat client  is null\n", __func__);
		goto end;
	}

	ret = bat_client->get_bat_id(bat_client->data,
					&val);
	if (ret) {
		pr_err("%s get bat vol fail %d\n", __func__, ret);
		val = ret;
		goto end;
	}

	ret = (val * 1000 / (data->batid_v_ref- val) ) * data->batid_r_pull/ 1000;
	data->bat_id = ret;

end:
	return snprintf(buf, PAGE_SIZE, "%d\n", data->bat_id);
}

static ssize_t bat_id_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	pr_info("%s un-supported\n", __func__);
	return count;
}
static DEVICE_ATTR(bat_id, S_IWUSR | S_IRUGO,
	bat_id_show, bat_id_store);

static ssize_t factory_kill_disable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;

	if (!chg_client) {
		pr_err("%s bat_client is null\n", __func__);
		ret = -ENODEV;
		goto end;
	}

	ret = chg_client->factory_kill_disable;

end:

	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t factory_kill_disable_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	int val;
	int ret;

	if (!chg_client) {
		pr_err("%s chg_client is null\n", __func__);
		goto end;
	}

	ret = kstrtoint(buf, 10, &val);
	if (ret) {
		pr_info("%s, %s not a valide buf(%d)\n", __func__, buf, ret);
		goto end;
	}

	chg_client->factory_kill_disable = !!val;

end:
	return count;
}
static DEVICE_ATTR(factory_kill_disable, S_IWUSR | S_IRUGO,
	factory_kill_disable_show, factory_kill_disable_store);

static ssize_t adc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	struct moto_chg_tcmd_client *client;
	int channel;
	int ret;

	channel = data->cur_adc_channel;
	if (channel < 100) {
		client = ap_adc_client;
	} else {
		client = pm_adc_client;
		channel -= 100;
	}

	if (!client) {
		pr_err("%s bat_client is null\n", __func__);
		ret = -ENODEV;
		goto end;
	}

	moto_chg_get_adc_value(data, client, data->cur_adc_channel, &ret);

end:
	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t adc_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	int val;
	int ret;

	ret = kstrtoint(buf, 10, &val);
	if (ret) {
		pr_info("%s, %s not a valide buf(%d)\n", __func__, buf, ret);
		goto end;
	}

	data->cur_adc_channel = data->adc_channel_list[val];

end:
	return count;
}

static DEVICE_ATTR(adc, S_IWUSR | S_IRUGO, adc_show, adc_store);

static ssize_t cpu_temp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	struct moto_chg_tcmd_client *client;
	int channel;
	int ret;

	channel = data->adc_channel_list[MOTO_ADC_TCMD_CHANNEL_CPU];
	if (channel < 100) {
		client = ap_adc_client;
	} else {
		client = pm_adc_client;
		channel -= 100;
	}

	if (!client) {
		pr_err("%s adc_client(%d) is null\n", __func__, channel);
		ret = -ENODEV;
		goto end;
	}

	moto_chg_get_adc_value(data, client, channel, &ret);
	ret = moto_chg_tcmd_res_to_temp(data, data->ntc_v_ref, ret, data->ntc_r_pull);

end:
	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t cpu_temp_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	pr_info("%s do not support store\n", __func__);
	return count;
}

static DEVICE_ATTR(cpu_temp, S_IWUSR | S_IRUGO,
				cpu_temp_show, cpu_temp_store);

static ssize_t pa_temp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	struct moto_chg_tcmd_client *client;
	int channel;
	int ret;

	channel = data->adc_channel_list[MOTO_ADC_TCMD_CHANNEL_PA];
	if (channel < 100) {
		client = ap_adc_client;
	} else {
		client = pm_adc_client;
		channel -= 100;
	}

	if (!client) {
		pr_err("%s pa_client(%d) is null\n", __func__, channel);
		ret = -ENODEV;
		goto end;
	}

	moto_chg_get_adc_value(data, client, channel, &ret);
	ret = moto_chg_tcmd_res_to_temp(data, data->ntc_v_ref, ret, data->ntc_r_pull);

end:
	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t pa_temp_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	pr_info("%s do not support store\n", __func__);
	return count;
}

static DEVICE_ATTR(pa_temp, S_IWUSR | S_IRUGO,
				pa_temp_show, pa_temp_store);

static ssize_t charger_temp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	struct moto_chg_tcmd_client *client;
	int channel;
	int ret;

	channel = data->adc_channel_list[MOTO_ADC_TCMD_CHANNEL_CHG];
	if (channel < 100) {
		client = ap_adc_client;
	} else {
		client = pm_adc_client;
		channel -= 100;
	}

	if (!client) {
		pr_err("%s adc client(%d) is null\n", __func__, channel);
		ret = -ENODEV;
		goto end;
	}

	moto_chg_get_adc_value(data, client, channel, &ret);
	ret = moto_chg_tcmd_res_to_temp(data, data->ntc_v_ref, ret, data->ntc_r_pull);

end:
	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t charger_temp_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	pr_info("%s do not support store\n", __func__);
	return count;
}

static DEVICE_ATTR(charger_temp, S_IWUSR | S_IRUGO,
				charger_temp_show, charger_temp_store);

static ssize_t adc_vbat_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct moto_chg_tcmd_data *data = platform_get_drvdata(pdev);
	struct moto_chg_tcmd_client *client;
	int channel;
	int ret;

	channel = data->adc_channel_list[MOTO_ADC_TCMD_CHANNEL_VBAT];
	if (channel < 100) {
		client = ap_adc_client;
	} else {
		client = pm_adc_client;
		channel -= 100;
	}

	if (!client) {
		pr_err("%s adc client(%s) is null\n", __func__,
			channel < 100 ? "ap" : "pm");
		ret = -ENODEV;
		goto end;
	}

	moto_chg_get_adc_value(data, client, channel, &ret);
	/*if ret is valid and its value is < 2500000 uv(the lowest battery voltage threshold)*/
	if ((ret > 0) && (ret < 2500000))
		ret *= 1000;

end:
	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t adc_vbat_store(struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	pr_info("%s do not support store\n", __func__);
	return count;
}

static DEVICE_ATTR(adc_vbat, S_IWUSR | S_IRUGO,
				adc_vbat_show, adc_vbat_store);

static ssize_t wireless_en_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	int val;

	if (!wls_client) {
		pr_err("%s wls_client is null\n", __func__);
		goto end;
	}

	ret = kstrtouint(buf, 10, &val);
	if (ret) {
		pr_info("%s, %s not a valide buf(%d)\n", __func__, buf, ret);
		goto end;
	}

	if (wls_client->wls_en) {
		ret = wls_client->wls_en(wls_client->data, !!val);
		if (ret) {
			pr_err("%s wls en fail %d\n", __func__, ret);
		}
	}

end:
	return count;
}

static DEVICE_ATTR(wireless_en, S_IRUGO|S_IWUSR,
				NULL, wireless_en_store);

static ssize_t wireless_chip_id_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	int val = 0;
	if (wls_client->get_chip_id) {
		val = wls_client->get_chip_id(wls_client->data);
		if (!val) {
			pr_err("%s wls get chip id fail %d\n", __func__, val);
		}
	}
	return sprintf(buf, "%x\n", val);
}
static DEVICE_ATTR(wireless_chip_id, S_IRUGO|S_IWUSR,
				wireless_chip_id_show, NULL);

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
	&dev_attr_bat_cycle.attr,
	&dev_attr_factory_kill_disable.attr,
	&dev_attr_chg_type.attr,
	&dev_attr_chg_watt.attr,
	&dev_attr_adc.attr,
	&dev_attr_cpu_temp.attr,
	&dev_attr_charger_temp.attr,
	&dev_attr_pa_temp.attr,
	&dev_attr_adc_vbat.attr,
	&dev_attr_wireless_en.attr,
	&dev_attr_wireless_chip_id.attr,
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
		case MOTO_CHG_TCMD_CLIENT_PM_ADC:
			pm_adc_client = client;
			break;
		case MOTO_CHG_TCMD_CLIENT_AP_ADC:
			ap_adc_client = client;
			break;
		case MOTO_CHG_TCMD_CLIENT_WLS:
			wls_client = client;
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
EXPORT_SYMBOL(moto_chg_tcmd_register);

static int moto_chg_tcmd_res_to_temp(struct moto_chg_tcmd_data *data,
					int vol, int vol_ntc, int r_pull)
{
	struct CHARGER_TEMPERATURE* table;
	int t_ntc = 80;
	int r_ntc;
	int i;

	table = data->ntc_table;
	if (table == NULL) {
		pr_err("%s no ntc table\n", __func__);
		return -20;
	}

	//(vol - vol_ntc) / vol_ntc = r_pull / r_ntc
	r_ntc = (vol_ntc * 1000 / (vol - vol_ntc) ) * r_pull / 1000;
	pr_info("%s r_ntc %d, v_ntc %d, v_ref %d, r_pull %d\n", __func__,
		r_ntc, vol_ntc, vol, r_pull);
	for(i=0;i<NTC_TABLE_VALID_NUM;i++) {
		if (r_ntc > table[i].TemperatureR) {
			t_ntc = table[i].CHARGER_Temp;
			break;
		}
	}

	return t_ntc;
}

static int moto_chg_tcmd_get_ntc_table(struct moto_chg_tcmd_data *data, int id)
{
	switch (id) {
		case 1:
			data->ntc_table = ntc_table1;
			break;
		default:
			data->ntc_table = ntc_table1;
			pr_info("%s do not find id %d. Using default ntc table\n", __func__, id);
			break;
	}

	return 0;
}

static int moto_chg_tcmd_parse_dt(struct moto_chg_tcmd_data *data)
{
	struct device_node *node = data->pdev->dev.of_node;
	char* channel_dt_list[MOTO_ADC_TCMD_CHANNEL_MAX] = {"mmi,adc-channel-cpu",
							"mmi,adc-channel-charger",
							"mmi,adc-channel-pa",
							"mmi,adc-channel-batid",
							"mmi,adc-channel-vbat"};
	int i;
	int val;
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

	for (i = 0; i < MOTO_ADC_TCMD_CHANNEL_MAX; i++) {
		ret = of_property_read_u32(node, channel_dt_list[i], &val);
		if (ret) {
			pr_err("%s donot find %s in dt\n", __func__, channel_dt_list[i]);
			val = -ENODEV;
		}
		data->adc_channel_list[i] = val;
	}

	ret = of_property_read_u32(node, "mmi,ntc_table", &val);
	if (ret) {
		pr_err("%s donot find ntc_table in dt\n", __func__);
		val = -ENODEV;
	}
	moto_chg_tcmd_get_ntc_table(data, val);

	ret = of_property_read_u32(node, "mmi,ntc_v_ref", &val);
	if (ret) {
		pr_err("%s donot find ntc v ref in dt\n", __func__);
		val = -ENODEV;
	}
	data->ntc_v_ref = val;

	ret = of_property_read_u32(node, "mmi,ntc_r_pull", &val);
	if (ret) {
		pr_err("%s donot find ntc r pull in dt\n", __func__);
		val = -ENODEV;
	}
	data->ntc_r_pull = val;

	ret = of_property_read_u32(node, "mmi,batid_v_ref", &val);
	if (ret) {
		pr_err("%s donot find batid v ref in dt\n", __func__);
		val = -ENODEV;
	}
	data->batid_v_ref = val;

	ret = of_property_read_u32(node, "mmi,batid_r_pull", &val);
	if (ret) {
		pr_err("%s donot find batid r pull in dt\n", __func__);
		val = -ENODEV;
	}
	data->batid_r_pull = val;

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
