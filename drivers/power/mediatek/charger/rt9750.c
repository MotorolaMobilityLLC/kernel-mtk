/*
 * Copyright (C) 2016 MediaTek Inc.
 * ShuFanLee <shufan_lee@richtek.com>
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kthread.h>

#include "mtk_charger_intf.h"
#include "rt9750.h"

#ifdef CONFIG_RT_REGMAP
#include <mt-plat/rt-regmap.h>
#endif

#define RT9750_DRV_VERSION	"1.0.6_MTK"

#define I2C_ACCESS_MAX_RETRY 5

struct rt9750_desc {
	int regmap_represent_slave_addr;
	const char *regmap_name;
	const char *chg_dev_name;
	const char *alias_name;
	const char *eint_name;
	u32 vbat_reg;
	u32 vout_reg;
	u32 iococp;
	u32 wdt;
};

static struct rt9750_desc rt9750_default_desc = {
	.regmap_represent_slave_addr = RT9750_SLAVE_ADDR,
	.regmap_name = "rt9750",
	.alias_name = "rt9750",
	.chg_dev_name = "load_switch",
	.eint_name = "rt9750_chr_1",
	.vbat_reg = 4400000,	/* uV */
	.vout_reg = 5000000,	/* uV */
	.iococp = 5000000,	/* uA */
	.wdt = 2000000,		/* us */
};

struct rt9750_info {
	struct i2c_client *i2c;
	struct rt9750_desc *desc;
	struct charger_device *chg_dev;
	struct charger_properties chg_props;
	struct mutex i2c_access_lock;
	struct mutex adc_access_lock;
	struct mutex gpio_access_lock;
	struct pinctrl *en_pinctrl;
	struct pinctrl_state *en_enable;
	struct pinctrl_state *en_disable;
	int irq;
	u8 chip_rev;
#if 0
	struct task_struct *task;
#endif
#ifdef CONFIG_RT_REGMAP
	struct rt_regmap_device *regmap_dev;
	struct rt_regmap_properties *regmap_prop;
#endif
};


static u32 rt9750_wdt[] = {
	0, 500000, 1000000, 2000000,
}; /* us */

static u8 rt9750_init_irq_data[] = {
	0x00, 0x00,
};

static u8 rt9750_maskall_irq_data[] = {
	0xEF, 0xFF,
};

/* ========= */
/* RT Regmap */
/* ========= */

#ifdef CONFIG_RT_REGMAP
RT_REG_DECL(RT9750_REG_CORE_CTRL0, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_EVENT1_MASK, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_EVENT2_MASK, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_EVENT1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_EVENT2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_EVENT1_EN, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_CONTROL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_ADC_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_SAMPLE_EN, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_PROT_DLYOCP, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_VBUS_OVP, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_VOUT_REG, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_VDROP_OVP, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_VDROP_ALM, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_VBAT_REG, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_IBUS_OCP, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_TBUS_OTP, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_TBAT_OTP, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_VBUS_ADC2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_VBUS_ADC1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_IBUS_ADC2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_IBUS_ADC1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_VOUT_ADC2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_VOUT_ADC1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_VDROP_ADC2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_VDROP_ADC1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_VBAT_ADC2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_VBAT_ADC1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_TBUS_ADC2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_TBUS_ADC1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_TBAT_ADC2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_TBAT_ADC1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_TDIE_ADC1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_EVENT_STATUS1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_EVENT_STATUS2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9750_REG_EVENT_STATUS, 1, RT_VOLATILE, {});

static rt_register_map_t rt9750_regmap_map[] = {
	RT_REG(RT9750_REG_CORE_CTRL0),
	RT_REG(RT9750_REG_EVENT1_MASK),
	RT_REG(RT9750_REG_EVENT2_MASK),
	RT_REG(RT9750_REG_EVENT1),
	RT_REG(RT9750_REG_EVENT2),
	RT_REG(RT9750_REG_EVENT1_EN),
	RT_REG(RT9750_REG_CONTROL),
	RT_REG(RT9750_REG_ADC_CTRL),
	RT_REG(RT9750_REG_SAMPLE_EN),
	RT_REG(RT9750_REG_PROT_DLYOCP),
	RT_REG(RT9750_REG_VBUS_OVP),
	RT_REG(RT9750_REG_VOUT_REG),
	RT_REG(RT9750_REG_VDROP_OVP),
	RT_REG(RT9750_REG_VDROP_ALM),
	RT_REG(RT9750_REG_VBAT_REG),
	RT_REG(RT9750_REG_IBUS_OCP),
	RT_REG(RT9750_REG_TBUS_OTP),
	RT_REG(RT9750_REG_TBAT_OTP),
	RT_REG(RT9750_REG_VBUS_ADC2),
	RT_REG(RT9750_REG_VBUS_ADC1),
	RT_REG(RT9750_REG_IBUS_ADC2),
	RT_REG(RT9750_REG_IBUS_ADC1),
	RT_REG(RT9750_REG_VOUT_ADC2),
	RT_REG(RT9750_REG_VOUT_ADC1),
	RT_REG(RT9750_REG_VDROP_ADC2),
	RT_REG(RT9750_REG_VDROP_ADC1),
	RT_REG(RT9750_REG_VBAT_ADC2),
	RT_REG(RT9750_REG_VBAT_ADC1),
	RT_REG(RT9750_REG_TBUS_ADC2),
	RT_REG(RT9750_REG_TBUS_ADC1),
	RT_REG(RT9750_REG_TBAT_ADC2),
	RT_REG(RT9750_REG_TBAT_ADC1),
	RT_REG(RT9750_REG_TDIE_ADC1),
	RT_REG(RT9750_REG_EVENT_STATUS1),
	RT_REG(RT9750_REG_EVENT_STATUS2),
	RT_REG(RT9750_REG_EVENT_STATUS),
};
#endif /* CONFIG_RT_REGMAP */

static const unsigned char rt9750_reg_addr[] = {
	RT9750_REG_CORE_CTRL0,
	RT9750_REG_EVENT1_MASK,
	RT9750_REG_EVENT2_MASK,
	RT9750_REG_EVENT1,
	RT9750_REG_EVENT2,
	RT9750_REG_EVENT1_EN,
	RT9750_REG_CONTROL,
	RT9750_REG_ADC_CTRL,
	RT9750_REG_SAMPLE_EN,
	RT9750_REG_PROT_DLYOCP,
	RT9750_REG_VBUS_OVP,
	RT9750_REG_VOUT_REG,
	RT9750_REG_VDROP_OVP,
	RT9750_REG_VDROP_ALM,
	RT9750_REG_VBAT_REG,
	RT9750_REG_IBUS_OCP,
	RT9750_REG_TBUS_OTP,
	RT9750_REG_TBAT_OTP,
	RT9750_REG_VBUS_ADC2,
	RT9750_REG_VBUS_ADC1,
	RT9750_REG_IBUS_ADC2,
	RT9750_REG_IBUS_ADC1,
	RT9750_REG_VOUT_ADC2,
	RT9750_REG_VOUT_ADC1,
	RT9750_REG_VDROP_ADC2,
	RT9750_REG_VDROP_ADC1,
	RT9750_REG_VBAT_ADC2,
	RT9750_REG_VBAT_ADC1,
	RT9750_REG_TBUS_ADC2,
	RT9750_REG_TBUS_ADC1,
	RT9750_REG_TBAT_ADC2,
	RT9750_REG_TBAT_ADC1,
	RT9750_REG_TDIE_ADC1,
	RT9750_REG_EVENT_STATUS1,
	RT9750_REG_EVENT_STATUS2,
	RT9750_REG_EVENT_STATUS,
};

/* ========================= */
/* I2C operations            */
/* ========================= */

static int rt9750_device_read(void *client, u32 addr, int leng, void *dst)
{
	int ret = 0;
	struct i2c_client *i2c = NULL;

	i2c = (struct i2c_client *)client;
	ret = i2c_smbus_read_i2c_block_data(i2c, addr, leng, dst);

	return ret;
}

static int rt9750_device_write(void *client, u32 addr, int leng,
	const void *src)
{
	int ret = 0;
	struct i2c_client *i2c = NULL;

	i2c = (struct i2c_client *)client;
	ret = i2c_smbus_write_i2c_block_data(i2c, addr, leng, src);

	return ret;
}

#ifdef CONFIG_RT_REGMAP
static struct rt_regmap_fops rt9750_regmap_fops = {
	.read_device = rt9750_device_read,
	.write_device = rt9750_device_write,
};

static int rt9750_register_rt_regmap(struct rt9750_info *info)
{
	int ret = 0;
	struct i2c_client *i2c = info->i2c;
	struct rt_regmap_properties *prop = NULL;

	pr_info("%s\n", __func__);

	prop = devm_kzalloc(&i2c->dev, sizeof(struct rt_regmap_properties),
		GFP_KERNEL);
	if (!prop)
		return -ENOMEM;

	prop->name = info->desc->regmap_name;
	prop->aliases = info->desc->regmap_name;
	prop->register_num = ARRAY_SIZE(rt9750_regmap_map);
	prop->rm = rt9750_regmap_map;
	prop->rt_regmap_mode = RT_SINGLE_BYTE | RT_CACHE_DISABLE |
		RT_IO_PASS_THROUGH;
	prop->io_log_en = 0;

	info->regmap_prop = prop;
	info->regmap_dev = rt_regmap_device_register_ex(
		info->regmap_prop,
		&rt9750_regmap_fops,
		&i2c->dev,
		i2c,
		info->desc->regmap_represent_slave_addr,
		info
	);

	if (!info->regmap_dev) {
		dev_err(&i2c->dev, "register regmap device failed\n");
		return -EIO;
	}

	return ret;
}
#endif /* CONFIG_RT_REGMAP */

static inline int _rt9750_i2c_write_byte(struct rt9750_info *info, u8 cmd,
	u8 data)
{
	int ret = 0, retry = 0;

	do {
#ifdef CONFIG_RT_REGMAP
		ret = rt_regmap_block_write(info->regmap_dev, cmd, 1, &data);
#else
		ret = rt9750_device_write(info->i2c, cmd, 1, &data);
#endif
		retry++;
		if (ret < 0)
			mdelay(20);
	} while (ret < 0 && retry < I2C_ACCESS_MAX_RETRY);

	if (ret < 0)
		pr_err("%s: I2CW[0x%02X] = 0x%02X failed\n", __func__,
			cmd, data);
	else
		pr_debug("%s: I2CW[0x%02X] = 0x%02X\n", __func__, cmd, data);

	return ret;
}

#if 0
static int rt9750_i2c_write_byte(struct rt9750_info *info, u8 cmd, u8 data)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = _rt9750_i2c_write_byte(info, cmd, data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}
#endif

static inline int _rt9750_i2c_read_byte(struct rt9750_info *info, u8 cmd)
{
	int ret = 0, ret_val = 0, retry = 0;

	do {
#ifdef CONFIG_RT_REGMAP
		ret = rt_regmap_block_read(info->regmap_dev, cmd, 1, &ret_val);
#else
		ret = rt9750_device_read(info->i2c, cmd, 1, &ret_val);
#endif
		retry++;
		if (ret < 0)
			msleep(20);
	} while (ret < 0 && retry < I2C_ACCESS_MAX_RETRY);

	if (ret < 0) {
		pr_err("%s: I2CR[0x%02X] failed\n", __func__, cmd);
		return ret;
	}

	ret_val = ret_val & 0xFF;

	pr_debug("%s: I2CR[0x%02X] = 0x%02X\n", __func__, cmd, ret_val);

	return ret_val;
}

static int rt9750_i2c_read_byte(struct rt9750_info *info, u8 cmd)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = _rt9750_i2c_read_byte(info, cmd);
	mutex_unlock(&info->i2c_access_lock);

	if (ret < 0)
		return ret;

	return (ret & 0xFF);
}

static inline int _rt9750_i2c_block_write(struct rt9750_info *info, u8 cmd,
	u32 leng, const u8 *data)
{
	int ret = 0;

#ifdef CONFIG_RT_REGMAP
	ret = rt_regmap_block_write(info->regmap_dev, cmd, leng, data);
#else
	ret = rt9750_device_write(info->i2c, cmd, leng, data);
#endif

	return ret;
}


static int rt9750_i2c_block_write(struct rt9750_info *info, u8 cmd, u32 leng,
	const u8 *data)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = _rt9750_i2c_block_write(info, cmd, leng, data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}

static inline int _rt9750_i2c_block_read(struct rt9750_info *info, u8 cmd,
	u32 leng, u8 *data)
{
	int ret = 0;

#ifdef CONFIG_RT_REGMAP
	ret = rt_regmap_block_read(info->regmap_dev, cmd, leng, data);
#else
	ret = rt9750_device_read(info->i2c, cmd, leng, data);
#endif

	return ret;
}


static int rt9750_i2c_block_read(struct rt9750_info *info, u8 cmd, u32 leng,
	u8 *data)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = _rt9750_i2c_block_read(info, cmd, leng, data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}

static int rt9750_i2c_test_bit(struct rt9750_info *info, u8 cmd, u8 shift)
{
	int ret = 0;

	ret = rt9750_i2c_read_byte(info, cmd);
	if (ret < 0)
		return ret;

	ret = ret & (1 << shift);

	return ret;
}

static int rt9750_i2c_update_bits(struct rt9750_info *info, u8 cmd, u8 data,
	u8 mask)
{
	int ret = 0;
	u8 reg_data = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = _rt9750_i2c_read_byte(info, cmd);
	if (ret < 0) {
		mutex_unlock(&info->i2c_access_lock);
		return ret;
	}

	reg_data = ret & 0xFF;
	reg_data &= ~mask;
	reg_data |= (data & mask);

	ret = _rt9750_i2c_write_byte(info, cmd, reg_data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}

static inline int rt9750_set_bit(struct rt9750_info *info, u8 reg, u8 mask)
{
	return rt9750_i2c_update_bits(info, reg, mask, mask);
}

static inline int rt9750_clr_bit(struct rt9750_info *info, u8 reg, u8 mask)
{
	return rt9750_i2c_update_bits(info, reg, 0x00, mask);
}

/* ========================= */
/* Internal function         */
/* ========================= */

static u8 rt9750_find_closest_reg_value(const u32 min, const u32 max,
	const u32 step, const u32 num, const u32 target)
{
	u32 i = 0, cur_val = 0, next_val = 0;

	/* Smaller than minimum supported value, use minimum one */
	if (target < min)
		return 0;

	for (i = 0; i < num - 1; i++) {
		cur_val = min + i * step;
		next_val = cur_val + step;

		if (cur_val > max)
			cur_val = max;

		if (next_val > max)
			next_val = max;

		if (target >= cur_val && target < next_val)
			return i;
	}

	/* Greater than maximum supported value, use maximum one */
	return num - 1;
}

static u8 rt9750_find_closest_reg_value_via_table(const u32 *value_table,
	const u32 table_size, const u32 target_value)
{
	u32 i = 0;

	/* Smaller than minimum supported value, use minimum one */
	if (target_value < value_table[0])
		return 0;

	for (i = 0; i < table_size - 1; i++) {
		if (target_value >= value_table[i] &&
		    target_value < value_table[i + 1])
			return i;
	}

	/* Greater than maximum supported value, use maximum one */
	return table_size - 1;
}

static u32 rt9750_find_closest_real_value(const u32 min, const u32 max,
	const u32 step, const u8 reg_val)
{
	u32 ret_val = 0;

	ret_val = min + reg_val * step;
	if (ret_val > max)
		ret_val = max;

	return ret_val;
}


static int rt9750_is_hw_exist(struct rt9750_info *info)
{
	int ret = 0;
	u8 dev_id = 0, chip_rev = 0;

	ret = i2c_smbus_read_byte_data(info->i2c, RT9750_REG_CORE_CTRL0);
	if (ret < 0)
		return false;

	dev_id = ret & 0x07;
	chip_rev = ret & 0x38;
	if (dev_id != RT9750_DEVICE_ID) {
		pr_err("%s: device id is incorrect\n", __func__);
		return false;
	}

	pr_info("%s: E%d(0x%02X)\n", __func__, chip_rev + 1, chip_rev);

	info->chip_rev = chip_rev;
	return true;
}

static irqreturn_t rt9750_irq_handler(int irq, void *data)
{
	int ret = 0, i = 0;
	struct rt9750_info *info = (struct rt9750_info *)data;
	u8 irq_data[2] = {0};
	u8 irq_mask[2] = {0};

	pr_info("%s\n", __func__);

	ret = rt9750_i2c_block_read(info, RT9750_REG_EVENT1_MASK, 2, irq_mask);
	if (ret < 0) {
		pr_err("%s: read irq mask failed\n", __func__);
		return IRQ_HANDLED;
	}

	ret = rt9750_i2c_block_read(info, RT9750_REG_EVENT1, 2, irq_data);
	if (ret < 0) {
		pr_err("%s: read irq data failed\n", __func__);
		return IRQ_HANDLED;
	}

	for (i = 0; i < 2; i++) {
		irq_data[i] &= ~irq_mask[i];
		pr_info("%s: event-%d(%d, %d, %d, %d, %d, %d, %d, %d)\n",
			__func__, i + 1,
			(irq_data[i] & 0x80) >> 7, (irq_data[i] & 0x40) >> 6,
			(irq_data[i] & 0x20) >> 5, (irq_data[i] & 0x10) >> 4,
			(irq_data[i] & 0x08) >> 3, (irq_data[i] & 0x04) >> 2,
			(irq_data[i] & 0x02) >> 1, irq_data[i] & 0x01);
	}

	return IRQ_HANDLED;
}

static int rt9750_register_irq(struct rt9750_info *info)
{
	int ret = 0;
	struct device_node *np = NULL;

	/* Parse irq number from dts */
	np = of_find_node_by_name(NULL, info->desc->eint_name);
	if (np)
		info->irq = irq_of_parse_and_map(np, 0);
	else {
		pr_err("%s: cannot get node\n", __func__);
		ret = -ENODEV;
		goto err_nodev;
	}
	pr_info("%s: irq = %d\n", __func__, info->irq);

	/* Request threaded IRQ */
	ret = request_threaded_irq(info->irq, NULL, rt9750_irq_handler,
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT, info->desc->eint_name,
		info);
	if (ret < 0) {
		pr_err("%s: request thread irq failed\n",
			__func__);
		goto err_request_irq;
	}

	return 0;

err_nodev:
err_request_irq:
	return ret;
}

static int rt9750_parse_dt(struct rt9750_info *info, struct device *dev)
{
	struct rt9750_desc *desc = NULL;
	struct device_node *np = dev->of_node;

	pr_info("%s\n", __func__);

	if (!np) {
		pr_err("%s: no device node\n", __func__);
		return -EINVAL;
	}

	info->desc = &rt9750_default_desc;
	desc = devm_kzalloc(dev, sizeof(struct rt9750_desc), GFP_KERNEL);
	if (!desc) {
		pr_err("%s: no enough memory\n", __func__);
		return -ENOMEM;
	}
	memcpy(desc, &rt9750_default_desc, sizeof(struct rt9750_desc));

	/*
	 * The following is how gpio is uesed on MTK's platform, "GPIO pinctrl"
	 * Please modify it if you have your own method
	 */
	info->en_pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(info->en_pinctrl)) {
		dev_err(dev, "%s: cannot find en pinctrl!\n", __func__);
		info->en_pinctrl = NULL;
	}

	if (info->en_pinctrl) {
		info->en_enable = pinctrl_lookup_state(info->en_pinctrl, "en_enable");
		if (IS_ERR(info->en_enable)) {
			dev_err(dev, "%s: cannot find en enable!\n", __func__);
			info->en_enable = NULL;
		}

		info->en_disable = pinctrl_lookup_state(info->en_pinctrl, "en_disable");
		if (IS_ERR(info->en_disable)) {
			dev_err(dev, "%s: cannot find en disable!\n", __func__);
			info->en_disable = NULL;
		}
	}

	if (of_property_read_string(np, "charger_name",
		&desc->chg_dev_name) < 0)
		pr_err("%s: no charger name\n", __func__);

	if (of_property_read_string(np, "eint_name", &desc->eint_name) < 0)
		pr_err("%s: no eint name\n", __func__);

	if (of_property_read_string(np, "regmap_name", &desc->regmap_name) < 0)
		pr_err("%s: no regmap name\n", __func__);

	if (of_property_read_string(np, "alias_name", &desc->alias_name) < 0)
		pr_err("%s: no alias name\n", __func__);

	if (of_property_read_u32(np, "vout_reg", &desc->vout_reg) < 0)
		pr_err("%s: no vout regulation\n", __func__);

	if (of_property_read_u32(np, "vbat_reg", &desc->vbat_reg) < 0)
		pr_err("%s: no vbat regulation\n", __func__);

	if (of_property_read_u32(np, "iococp", &desc->iococp) < 0)
		pr_err("%s: no iococp\n", __func__);

	if (of_property_read_u32(np, "wdt", &desc->wdt) < 0)
		pr_err("%s: no wdt\n", __func__);

	info->desc = desc;
	info->chg_props.alias_name = info->desc->alias_name;
	pr_info("%s: chg_name:%s alias:%s\n", __func__,
		info->desc->chg_dev_name, info->chg_props.alias_name);

	return 0;
}

static int rt9750_set_wdt(struct rt9750_info *info, const u32 us)
{
	int ret = 0;
	u8 wdt_reg = 0;

	wdt_reg = rt9750_find_closest_reg_value_via_table(rt9750_wdt,
		ARRAY_SIZE(rt9750_wdt), us);

	pr_info("%s: set wdt = %dms(0x%02X)\n", __func__, us / 1000, wdt_reg);

	ret = rt9750_i2c_update_bits(
		info,
		RT9750_REG_CONTROL,
		wdt_reg << RT9750_SHIFT_WDT,
		RT9750_MASK_WDT
	);

	return ret;
}

static int rt9750_get_vout(struct rt9750_info *info, u32 *vout)
{
	int ret = 0;
	u8 reg_vout = 0;

	ret = rt9750_i2c_read_byte(info, RT9750_REG_VOUT_REG);
	if (ret < 0)
		return ret;
	reg_vout = ((ret & RT9750_MASK_VOUT) >> RT9750_SHIFT_VOUT) & 0xFF;

	*vout = rt9750_find_closest_real_value(RT9750_VOUT_MIN, RT9750_VOUT_MAX,
		RT9750_VOUT_STEP, reg_vout);

	return ret;
}

static int rt9750_set_vout(struct rt9750_info *info, u32 uV)
{
	int ret = 0;
	u8 reg_vout = 0;

	reg_vout = rt9750_find_closest_reg_value(
		RT9750_VOUT_MIN,
		RT9750_VOUT_MAX,
		RT9750_VOUT_STEP,
		RT9750_VOUT_NUM,
		uV
	);

	pr_info("%s: vout = %d (0x%02X)\n", __func__, uV, reg_vout);

	ret = rt9750_i2c_update_bits(
		info,
		RT9750_REG_VOUT_REG,
		reg_vout << RT9750_SHIFT_VOUT,
		RT9750_MASK_VOUT
	);

	return ret;
}

static int rt9750_get_vbat(struct rt9750_info *info, u32 *vbat)
{
	int ret = 0;
	u8 reg_vbat = 0;

	ret = rt9750_i2c_read_byte(info, RT9750_REG_VBAT_REG);
	if (ret < 0)
		return ret;
	reg_vbat = ((ret & RT9750_MASK_VBAT) >> RT9750_SHIFT_VBAT) & 0xFF;

	*vbat = rt9750_find_closest_real_value(RT9750_VBAT_MIN, RT9750_VBAT_MAX,
		RT9750_VBAT_STEP, reg_vbat);

	return ret;
}

static int rt9750_set_vbat(struct rt9750_info *info, u32 uV)
{
	int ret = 0;
	u8 reg_vbat = 0;

	reg_vbat = rt9750_find_closest_reg_value(
		RT9750_VBAT_MIN,
		RT9750_VBAT_MAX,
		RT9750_VBAT_STEP,
		RT9750_VBAT_NUM,
		uV
	);

	pr_info("%s: vbat = %d (0x%02X)\n", __func__, uV, reg_vbat);

	ret = rt9750_i2c_update_bits(
		info,
		RT9750_REG_VBAT_REG,
		reg_vbat << RT9750_SHIFT_VBAT,
		RT9750_MASK_VBAT
	);

	return ret;
}

static int rt9750_set_iococp(struct rt9750_info *info, u32 uA)
{
	int ret = 0;
	u8 reg_iococp = 0;

	reg_iococp = rt9750_find_closest_reg_value(RT9750_IOCOCP_MIN,
		RT9750_IOCOCP_MAX, RT9750_IOCOCP_STEP, RT9750_IOCOCP_NUM, uA);

	pr_info("%s: iococp = %d (0x%02X)\n", __func__, uA, reg_iococp);

	ret = rt9750_i2c_update_bits(
		info,
		RT9750_REG_PROT_DLYOCP,
		reg_iococp << RT9750_SHIFT_IOCOCP,
		RT9750_MASK_IOCOCP
	);

	return ret;
}

static int rt9750_mask_all_irq(struct rt9750_info *info, bool maskall)
{
	int ret = 0;

	pr_info("%s: mask all = %d\n", __func__, maskall);

	if (maskall)
		ret = rt9750_i2c_block_write(info, RT9750_REG_EVENT1_MASK,
			ARRAY_SIZE(rt9750_maskall_irq_data),
			rt9750_maskall_irq_data);
	else
		ret = rt9750_i2c_block_write(info, RT9750_REG_EVENT1_MASK,
			ARRAY_SIZE(rt9750_init_irq_data),
			rt9750_init_irq_data);

	return ret;
}

static int rt9750_init_setting(struct rt9750_info *info)
{
	int ret = 0;

	ret = rt9750_mask_all_irq(info, true);

	ret = rt9750_set_wdt(info, info->desc->wdt);
	if (ret < 0)
		pr_err("%s: set wdt failed\n", __func__);

	ret = rt9750_set_vout(info, info->desc->vout_reg);
	if (ret < 0)
		pr_err("%s: set vout failed\n", __func__);

	ret = rt9750_set_vbat(info, info->desc->vbat_reg);
	if (ret < 0)
		pr_err("%s: set vbat failed\n", __func__);

	ret = rt9750_set_iococp(info, info->desc->iococp);
	if (ret < 0)
		pr_err("%s: set iococp failed\n", __func__);

	return ret;
}

static int rt9750_is_switch_enable(struct rt9750_info *info, bool *en)
{
	int ret = 0;

	ret = rt9750_i2c_test_bit(info, RT9750_REG_CONTROL,
		RT9750_SHIFT_CHG_EN);
	if (ret < 0) {
		*en = false;
		return ret;
	}

	*en = (ret == 0 ? false : true);
	pr_info("%s: enable = %d\n", __func__, *en);

	return ret;
}

#if 0
static int rt9750_get_vbus_adc(struct rt9750_info *info, u32 *vbus_adc)
{
	int ret = 0;
	u8 data[2] = {0};

	ret = rt9750_i2c_block_read(info, RT9750_REG_VBUS_ADC2, 2, data);
	if (ret < 0) {
		pr_err("%s: get vbus adc failed\n", __func__);
		return ret;
	}

	*vbus_adc = ((data[0] & RT9750_MASK_VBUS_ADC2) << 8) + data[1];

	pr_info("%s: vbus_adc = %dmV\n", __func__, *vbus_adc);
	return ret;
}
static int rt9750_get_vout_adc(struct rt9750_info *info, u32 *vout_adc)
{
	int ret = 0;
	u8 data[2] = {0};

	ret = rt9750_i2c_block_read(info, RT9750_REG_VOUT_ADC2, 2, data);
	if (ret < 0) {
		pr_err("%s: get vout adc failed\n", __func__);
		return ret;
	}

	*vout_adc = ((data[0] & RT9750_MASK_VOUT_ADC2) << 8) + data[1];

	pr_info("%s: vout_adc = %dmV\n", __func__, *vout_adc);
	return ret;
}

static int rt9750_get_vdrop_adc(struct rt9750_info *info, u32 *vdrop_adc)
{
	int ret = 0;
	u8 data[2] = {0};

	ret = rt9750_i2c_block_read(info, RT9750_REG_VDROP_ADC2, 2, data);
	if (ret < 0) {
		pr_err("%s: get vdrop adc failed\n", __func__);
		return ret;
	}

	*vdrop_adc = ((data[0] & RT9750_MASK_VDROP_ADC2) << 8) + data[1];

	pr_info("%s: vdrop_adc = %dmV\n", __func__, *vdrop_adc);
	return ret;
}

static int rt9750_get_vbat_adc(struct rt9750_info *info, u32 *vbat_adc)
{
	int ret = 0;
	u8 data[2] = {0};

	ret = rt9750_i2c_block_read(info, RT9750_REG_VBAT_ADC2, 2, data);
	if (ret < 0) {
		pr_err("%s: get vbat adc failed\n", __func__);
		return ret;
	}

	*vbat_adc = ((data[0] & RT9750_MASK_VBAT_ADC2) << 8) + data[1];

	pr_info("%s: vbat_adc = %dmV\n", __func__, *vbat_adc);
	return ret;
}


static int rt9750_get_tbat_adc(struct rt9750_info *info, u32 *tbat_adc)
{
	int ret = 0;
	u8 data[2] = {0};

	ret = rt9750_i2c_block_read(info, RT9750_REG_TBAT_ADC2, 2, data);
	if (ret < 0) {
		pr_err("%s: get tbat adc failed\n", __func__);
		return ret;
	}

	*tbat_adc = ((data[0] & RT9750_MASK_TBAT_ADC2) << 8) + data[1];

	pr_info("%s: tbat_adc = %ddegree\n", __func__, *tbat_adc);
	return ret;
}

static int rt9750_get_tbus_adc(struct rt9750_info *info, u32 *tbus_adc)
{
	int ret = 0;
	u8 data[2] = {0};

	ret = rt9750_i2c_block_read(info, RT9750_REG_TBUS_ADC2, 2, data);
	if (ret < 0) {
		pr_err("%s: get tbus adc failed\n", __func__);
		return ret;
	}

	*tbus_adc = ((data[0] & RT9750_MASK_TBUS_ADC2) << 8) + data[1];

	pr_info("%s: tbus_adc = %ddegree\n", __func__, *tbus_adc);
	return ret;
}
#endif

/* ========================= */
/* Released function         */
/* ========================= */

static int rt9750_dump_register(struct charger_device *chg_dev)
{
	int i = 0, ret = 0;
	u32 vout = 0, vbat = 0;
	bool en = false;
	struct rt9750_info *info = dev_get_drvdata(&chg_dev->dev);

	ret = rt9750_get_vout(info, &vout);
	ret = rt9750_get_vbat(info, &vbat);
	ret = rt9750_is_switch_enable(info, &en);

	for (i = 0; i < ARRAY_SIZE(rt9750_reg_addr); i++) {
		ret = rt9750_i2c_read_byte(info, rt9750_reg_addr[i]);
		if (ret < 0)
			return ret;
	}

	pr_info("%s: VOUT = %dmV, VBAT = %dmV, SWITCH_EN = %d\n",
		__func__, vout / 1000, vbat / 1000, en);

	return ret;
}

static int _rt9750_enable_chip(struct rt9750_info *info, bool en)
{
	pr_info("%s\n", __func__);
	if (!info->en_pinctrl || !info->en_enable || !info->en_disable) {
		pr_info("%s: no pinctrl\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&info->gpio_access_lock);
	if (en) {
		/* Lock I2C to solve I2C SDA drop problem */
		i2c_lock_adapter(info->i2c->adapter);
		pinctrl_select_state(info->en_pinctrl, info->en_enable);
		pr_info("%s: set gpio high\n", __func__);
		udelay(10);
		i2c_unlock_adapter(info->i2c->adapter);
	} else {
		pinctrl_select_state(info->en_pinctrl, info->en_disable);
		pr_info("%s: set gpio low\n", __func__);
	}

	mutex_unlock(&info->gpio_access_lock);
	return 0;

}

static int rt9750_enable_chip(struct charger_device *chg_dev, bool en)
{
	int ret = 0;
	struct rt9750_info *info = dev_get_drvdata(&chg_dev->dev);

	ret = _rt9750_enable_chip(info, en);
	if (ret < 0) {
		pr_err("%s: enable chip failed\n", __func__);
		return ret;
	}

	if (en) {
		ret = rt9750_init_setting(info);
		if (ret < 0)
			pr_err("%s: init setting failed\n", __func__);
	}

	return ret;
}

static int rt9750_enable_switch(struct charger_device *chg_dev, bool en)
{
	int ret = 0;
	struct rt9750_info *info = dev_get_drvdata(&chg_dev->dev);

	pr_info("%s, enable = %d\n", __func__, en);
	ret = (en ? rt9750_set_bit : rt9750_clr_bit)
		(info, RT9750_REG_CONTROL, RT9750_MASK_CHG_EN);

	return ret;
}

static int rt9750_set_ibusoc(struct charger_device *chg_dev, u32 uA)
{
	int ret = 0;
	struct rt9750_info *info = dev_get_drvdata(&chg_dev->dev);
	u8 reg_ibusoc = 0;

	reg_ibusoc = rt9750_find_closest_reg_value(RT9750_IBUSOC_MIN,
		RT9750_IBUSOC_MAX, RT9750_IBUSOC_STEP, RT9750_IBUSOC_NUM, uA);

	pr_info("%s: ibusoc = %d (0x%02X)\n", __func__, uA, reg_ibusoc);

	ret = rt9750_i2c_update_bits(
		info,
		RT9750_REG_IBUS_OCP,
		reg_ibusoc << RT9750_SHIFT_IBUS_OCP,
		RT9750_MASK_IBUS_OCP
	);

	return ret;
}


static int rt9750_set_vbusov(struct charger_device *chg_dev, u32 uV)
{
	int ret = 0;
	struct rt9750_info *info = dev_get_drvdata(&chg_dev->dev);
	u8 reg_vbusov = 0;

	reg_vbusov = rt9750_find_closest_reg_value(RT9750_VBUSOV_MIN,
		RT9750_VBUSOV_MAX, RT9750_VBUSOV_STEP, RT9750_VBUSOV_NUM,
		uV);

	pr_info("%s: vbusov = %d (0x%02X)\n", __func__, uV, reg_vbusov);

	ret = rt9750_i2c_update_bits(
		info,
		RT9750_REG_VBUS_OVP,
		reg_vbusov << RT9750_SHIFT_VBUSOVP,
		RT9750_MASK_VBUSOVP
	);

	return ret;
}


static int rt9750_kick_wdt(struct charger_device *chg_dev)
{
	int ret = 0;
	u32 vout = 0;
	struct rt9750_info *info = dev_get_drvdata(&chg_dev->dev);

	/* Any I2C operation can kick wdt */
	pr_info("%s\n", __func__);
	ret = rt9750_get_vout(info, &vout);

	return ret;
}

static int rt9750_get_ibus_adc(struct charger_device *chg_dev, u32 *ibus_adc)
{
	int ret = 0;
	u8 data[2] = {0};
	struct rt9750_info *info = dev_get_drvdata(&chg_dev->dev);

	ret = rt9750_i2c_block_read(info, RT9750_REG_IBUS_ADC2, 2, data);
	if (ret < 0) {
		pr_err("%s: get ibus adc failed\n", __func__);
		return ret;
	}

	*ibus_adc = ((data[0] & RT9750_MASK_IBUS_ADC2) << 8) + data[1];

	pr_info("%s: ibus_adc = %dmA\n", __func__, *ibus_adc);
	return ret;
}

static int rt9750_get_tdie_adc(struct charger_device *chg_dev,
	int *tdie_adc_min, int *tdie_adc_max)
{
	int ret = 0;
	struct rt9750_info *info = dev_get_drvdata(&chg_dev->dev);

	ret = rt9750_i2c_read_byte(info, RT9750_REG_TDIE_ADC1);
	if (ret < 0) {
		pr_err("%s: get vbus adc failed\n", __func__);
		return ret;
	}

	*tdie_adc_min = ret;
	*tdie_adc_max = ret;

	pr_info("%s: tdie_adc = %ddegree\n", __func__, *tdie_adc_min);
	return ret;
}

static struct charger_ops rt9750_chg_ops = {
	.enable_chip = rt9750_enable_chip,
	.enable_direct_charging = rt9750_enable_switch,
	.dump_registers = rt9750_dump_register,
	.kick_direct_charging_wdt = rt9750_kick_wdt,
	.set_direct_charging_ibusoc = rt9750_set_ibusoc,
	.set_direct_charging_vbusov = rt9750_set_vbusov,
	.get_ibus_adc = rt9750_get_ibus_adc,
	.get_tchg_adc = rt9750_get_tdie_adc,
};


/* ========================= */
/* I2C driver function       */
/* ========================= */

#if 0
static int rt9750_dbg_thread(void *data)
{
	int ret = 0;
	u32 ibus_adc = 0;
	int tdie_min = 0, tdie_max = 0;
	bool en = false;
	struct rt9750_info *info = (struct rt9750_info *)data;

	pr_info("%s\n", __func__);
	ret = rt9750_enable_chip(info->chg_dev, true);
	while (1) {
		ret = rt9750_get_ibus_adc(info->chg_dev, &ibus_adc);
		ret = rt9750_get_tdie_adc(info->chg_dev, &tdie_min, &tdie_max);
		ret = rt9750_set_ibusoc(info->chg_dev, 6000000);
		ret = rt9750_set_vbusov(info->chg_dev, 6000000);
		ret = rt9750_is_switch_enable(info, &en);
		msleep(2000);
	};

	return ret;
}
#endif

static int rt9750_probe(struct i2c_client *i2c,
	const struct i2c_device_id *dev_id)
{
	int ret = 0;
	struct rt9750_info *info = NULL;

	pr_info("%s: %s\n", __func__, RT9750_DRV_VERSION);

	info = devm_kzalloc(&i2c->dev, sizeof(struct rt9750_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->i2c = i2c;
	mutex_init(&info->i2c_access_lock);
	mutex_init(&info->adc_access_lock);
	mutex_init(&info->gpio_access_lock);

	ret = rt9750_parse_dt(info, &i2c->dev);
	if (ret < 0) {
		pr_err("%s: parse dt failed\n", __func__);
		goto err_parse_dt;
	}

	/* Register charger device */
	info->chg_dev = charger_device_register(info->desc->chg_dev_name,
		&i2c->dev, info, &rt9750_chg_ops, &info->chg_props);
	if (IS_ERR_OR_NULL(info->chg_dev)) {
		ret = PTR_ERR(info->chg_dev);
		goto err_register_chg_dev;
	}

	/* Enable Chip */
	ret = _rt9750_enable_chip(info, true);
	if (ret < 0) {
		pr_err("%s: enable chip failed\n", __func__);
		goto err_enable_chip;
	}

	/* Is HW exist */
	if (!rt9750_is_hw_exist(info)) {
		pr_err("%s: no rt9750 exists\n", __func__);
		ret = -ENODEV;
		goto err_no_dev;
	}
	i2c_set_clientdata(i2c, info);

#ifdef CONFIG_RT_REGMAP
	ret = rt9750_register_rt_regmap(info);
	if (ret < 0)
		goto err_register_regmap;
#endif

	ret = rt9750_register_irq(info);
	if (ret < 0) {
		pr_err("%s: register irq failed\n", __func__);
		goto err_register_irq;
	}
	rt9750_dump_register(info->chg_dev);

	/* Disable Chip */
	ret = _rt9750_enable_chip(info, false);
	if (ret < 0) {
		pr_err("%s: disable chip failed\n", __func__);
		goto err_disable_chip;
	}

#if 0
	info->task = kthread_create(rt9750_dbg_thread, (void *)info,
		"dbg_thread");
	if (IS_ERR(info->task))
		pr_err("%s: create dbg thread failed\n", __func__);
	wake_up_process(info->task);
#endif


	return ret;

err_register_irq:
err_disable_chip:
#ifdef CONFIG_RT_REGMAP
	rt_regmap_device_unregister(info->regmap_dev);
err_register_regmap:
#endif
err_no_dev:
	_rt9750_enable_chip(info, false);
err_enable_chip:
	charger_device_unregister(info->chg_dev);
err_parse_dt:
err_register_chg_dev:
	mutex_destroy(&info->i2c_access_lock);
	mutex_destroy(&info->adc_access_lock);
	mutex_destroy(&info->gpio_access_lock);
	return ret;
}


static int rt9750_remove(struct i2c_client *i2c)
{
	int ret = 0;
	struct rt9750_info *info = i2c_get_clientdata(i2c);

	pr_info("%s\n", __func__);

	if (info) {
#ifdef CONFIG_RT_REGMAP
		rt_regmap_device_unregister(info->regmap_dev);
#endif
		charger_device_unregister(info->chg_dev);
		mutex_destroy(&info->i2c_access_lock);
		mutex_destroy(&info->adc_access_lock);
		mutex_destroy(&info->gpio_access_lock);
	}

	return ret;
}

static void rt9750_shutdown(struct i2c_client *i2c)
{
	pr_info("%s\n", __func__);
}

static const struct i2c_device_id rt9750_i2c_id[] = {
	{"rt9750", 0},
	{}
};

#ifdef CONFIG_OF
static const struct of_device_id rt9750_of_match[] = {
	{ .compatible = "mediatek,slave_charger", },
	{},
};
#else /* Not define CONFIG_OF */

#define RT9750_BUSNUM 0

static struct i2c_board_info rt9750_i2c_board_info __initdata = {
	I2C_BOARD_INFO("rt9750", rt9750_SALVE_ADDR)
};
#endif /* CONFIG_OF */


static struct i2c_driver rt9750_i2c_driver = {
	.driver = {
		.name = "rt9750",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = rt9750_of_match,
#endif
	},
	.probe = rt9750_probe,
	.remove = rt9750_remove,
	.shutdown = rt9750_shutdown,
	.id_table = rt9750_i2c_id,
};

static int __init rt9750_init(void)
{
	int ret = 0;

#ifdef CONFIG_OF
	pr_info("%s: with dts\n", __func__);
#else
	pr_info("%s: without dts\n", __func__);
	i2c_register_board_info(RT9750_BUSNUM, &rt9750_i2c_board_info, 1);
#endif

	ret = i2c_add_driver(&rt9750_i2c_driver);
	if (ret < 0)
		pr_err("%s: register i2c driver failed\n", __func__);

	return ret;
}
module_init(rt9750_init);


static void __exit rt9750_exit(void)
{
	i2c_del_driver(&rt9750_i2c_driver);
}
module_exit(rt9750_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("ShuFanLee <shufan_lee@richtek.com>");
MODULE_DESCRIPTION("RT9750 Load Switch Driver");
MODULE_VERSION(RT9750_DRV_VERSION);

/*
 * Version Note
 * 1.0.6
 * (1) Disable chip if probed failed
 *
 * 1.0.5
 * (1) Unregister charger device if probe failed
 * (2) Mask all irqs for now
 *
 * 1.0.4
 * (1) Add set_iococp
 * (2) Init iococp in init_setting
 *
 * 1.0.3
 * (1) Remove registering power supply class
 *
 * 1.0.2
 * (1) Add interface for setting ibusoc/vbusov
 * (2) Write initial setting every time after enabling chip
 * (3) Init vout_reg/vbat_reg in init_setting
 *
 * 1.0.1
 * (1) Add gpio lock for rt9750_enable_chip
 *
 * 1.0.0
 * First Release
 */
