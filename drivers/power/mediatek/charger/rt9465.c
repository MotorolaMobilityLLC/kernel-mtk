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

#ifdef CONFIG_RT_REGMAP
#include <mt-plat/rt-regmap.h>
#endif

#ifdef CONFIG_RT9465_PWR_EN_TO_MT6336
#include <mt6336.h>
#endif
#include "mtk_charger_intf.h"
#include "rt9465.h"
#define I2C_ACCESS_MAX_RETRY	5
#define RT9465_DRV_VERSION	"1.0.6_MTK"

/* ======================= */
/* RT9465 Parameter        */
/* ======================= */

enum rt9465_charging_status {
	RT9465_CHG_STATUS_READY = 0,
	RT9465_CHG_STATUS_PROGRESS,
	RT9465_CHG_STATUS_DONE,
	RT9465_CHG_STATUS_FAULT,
	RT9465_CHG_STATUS_MAX,
};

/* Charging status name */
static const char *rt9465_chg_status_name[RT9465_CHG_STATUS_MAX] = {
	"ready", "progress", "done", "fault",
};

static const u8 rt9465_hidden_mode_addr[] = {
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
};

static const u8 rt9465_hidden_mode_val[] = {
	0x30, 0x26, 0xC7, 0xA4, 0x33, 0x06, 0x5F, 0xE1,
};

struct rt9465_desc {
	u32 ichg;		/* uA */
	u32 mivr;		/* uV */
	u32 cv;			/* uV */
	u32 ieoc;		/* uA */
	u32 safety_timer;	/* hour */
	bool enable_te;
	bool enable_wdt;
	int regmap_represent_slave_addr;
	const char *regmap_name;
	const char *chg_dev_name;
	const char *alias_name;
	const char *eint_name;
};

/* These default values will be used if there's no property in dts */
static struct rt9465_desc rt9465_default_desc = {
	.ichg = 2000000,	/* uA */
	.mivr = 4400000,	/* uV */
	.cv = 4350000,		/* uV */
	.ieoc = 250000,		/* uA */
	.safety_timer = 12,	/* hour */
	.enable_te = true,
	.enable_wdt = true,
	.regmap_represent_slave_addr = RT9465_SLAVE_ADDR,
	.regmap_name = "rt9465",
	.chg_dev_name = "secondary_chg",
	.alias_name = "rt9465",
	.eint_name = "rt9465_slave_chr",
};

struct rt9465_info {
	struct i2c_client *i2c;
	struct rt9465_desc *desc;
	struct charger_device *chg_dev;
	struct charger_properties chg_props;
	struct device *dev;
	struct mutex i2c_access_lock;
	struct mutex adc_access_lock;
	struct mutex gpio_access_lock;
	struct pinctrl *en_pinctrl;
	struct pinctrl_state *en_enable;
	struct pinctrl_state *en_disable;
	int irq;
	u8 device_id;
#ifdef CONFIG_RT_REGMAP
	struct rt_regmap_device *regmap_dev;
	struct rt_regmap_properties *regmap_prop;
#endif
#ifdef CONFIG_RT9465_PWR_EN_TO_MT6336
	struct mt6336_ctrl *lowq_ctrl;
#endif
};


/* ======================= */
/* Address & Default value */
/* ======================= */


static const unsigned char rt9465_reg_addr[] = {
	RT9465_REG_CHG_CTRL0,
	RT9465_REG_CHG_CTRL1,
	RT9465_REG_CHG_CTRL2,
	RT9465_REG_CHG_CTRL3,
	RT9465_REG_CHG_CTRL4,
	RT9465_REG_CHG_CTRL5,
	RT9465_REG_CHG_CTRL6,
	RT9465_REG_CHG_CTRL7,
	RT9465_REG_CHG_CTRL8,
	RT9465_REG_CHG_CTRL9,
	RT9465_REG_CHG_CTRL10,
	RT9465_REG_CHG_CTRL12,
	RT9465_REG_CHG_CTRL13,
	RT9465_REG_HIDDEN_CTRL2,
	RT9465_REG_HIDDEN_CTRL6,
	RT9465_REG_SYSTEM1,
	RT9465_REG_CHG_STATC,
	RT9465_REG_CHG_FAULT,
	RT9465_REG_CHG_IRQ1,
	RT9465_REG_CHG_IRQ2,
	RT9465_REG_CHG_STATC_MASK,
	RT9465_REG_CHG_FAULT_MASK,
	RT9465_REG_CHG_IRQ1_MASK,
	RT9465_REG_CHG_IRQ2_MASK,
};

/* ========= */
/* RT Regmap */
/* ========= */

#ifdef CONFIG_RT_REGMAP
RT_REG_DECL(RT9465_REG_CHG_CTRL0, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_CTRL1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_CTRL2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_CTRL3, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_CTRL4, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_CTRL5, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_CTRL6, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_CTRL7, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_CTRL8, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_CTRL9, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_CTRL10, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_CTRL12, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_CTRL13, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_HIDDEN_CTRL2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_HIDDEN_CTRL6, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_SYSTEM1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_STATC, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_FAULT, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_IRQ1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_IRQ2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_STATC_MASK, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_FAULT_MASK, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_IRQ1_MASK, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9465_REG_CHG_IRQ2_MASK, 1, RT_VOLATILE, {});

static rt_register_map_t rt9465_regmap_map[] = {
	RT_REG(RT9465_REG_CHG_CTRL0),
	RT_REG(RT9465_REG_CHG_CTRL1),
	RT_REG(RT9465_REG_CHG_CTRL2),
	RT_REG(RT9465_REG_CHG_CTRL3),
	RT_REG(RT9465_REG_CHG_CTRL4),
	RT_REG(RT9465_REG_CHG_CTRL5),
	RT_REG(RT9465_REG_CHG_CTRL6),
	RT_REG(RT9465_REG_CHG_CTRL7),
	RT_REG(RT9465_REG_CHG_CTRL8),
	RT_REG(RT9465_REG_CHG_CTRL9),
	RT_REG(RT9465_REG_CHG_CTRL10),
	RT_REG(RT9465_REG_CHG_CTRL12),
	RT_REG(RT9465_REG_CHG_CTRL13),
	RT_REG(RT9465_REG_HIDDEN_CTRL2),
	RT_REG(RT9465_REG_HIDDEN_CTRL6),
	RT_REG(RT9465_REG_SYSTEM1),
	RT_REG(RT9465_REG_CHG_STATC),
	RT_REG(RT9465_REG_CHG_FAULT),
	RT_REG(RT9465_REG_CHG_IRQ1),
	RT_REG(RT9465_REG_CHG_IRQ2),
	RT_REG(RT9465_REG_CHG_STATC_MASK),
	RT_REG(RT9465_REG_CHG_FAULT_MASK),
	RT_REG(RT9465_REG_CHG_IRQ1_MASK),
	RT_REG(RT9465_REG_CHG_IRQ2_MASK),
};
#endif /* CONFIG_RT_REGMAP */

/* ========================= */
/* I2C operations            */
/* ========================= */

static int rt9465_device_read(void *client, u32 addr, int leng, void *dst)
{
	int ret = 0;
	struct i2c_client *i2c = NULL;

	i2c = (struct i2c_client *)client;
	ret = i2c_smbus_read_i2c_block_data(i2c, addr, leng, dst);

	return ret;
}

static int rt9465_device_write(void *client, u32 addr, int leng,
	const void *src)
{
	int ret = 0;
	struct i2c_client *i2c = NULL;

	i2c = (struct i2c_client *)client;
	ret = i2c_smbus_write_i2c_block_data(i2c, addr, leng, src);

	return ret;
}

#ifdef CONFIG_RT_REGMAP
static struct rt_regmap_fops rt9465_regmap_fops = {
	.read_device = rt9465_device_read,
	.write_device = rt9465_device_write,
};

static int rt9465_register_rt_regmap(struct rt9465_info *info)
{
	int ret = 0;
	struct i2c_client *i2c = info->i2c;
	struct rt_regmap_properties *prop = NULL;

	dev_info(info->dev, "%s\n", __func__);

	prop = devm_kzalloc(&i2c->dev, sizeof(struct rt_regmap_properties),
		GFP_KERNEL);
	if (!prop)
		return -ENOMEM;

	prop->name = info->desc->regmap_name;
	prop->aliases = info->desc->regmap_name;
	prop->register_num = ARRAY_SIZE(rt9465_regmap_map);
	prop->rm = rt9465_regmap_map;
	prop->rt_regmap_mode = RT_SINGLE_BYTE | RT_CACHE_DISABLE |
		RT_IO_PASS_THROUGH;
	prop->io_log_en = 0;

	info->regmap_prop = prop;
	info->regmap_dev = rt_regmap_device_register_ex(
		info->regmap_prop,
		&rt9465_regmap_fops,
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

static inline int _rt9465_i2c_write_byte(struct rt9465_info *info, u8 cmd,
	u8 data)
{
	int ret = 0, retry = 0;

	do {
#ifdef CONFIG_RT_REGMAP
		ret = rt_regmap_block_write(info->regmap_dev, cmd, 1, &data);
#else
		ret = rt9465_device_write(info->i2c, cmd, 1, &data);
#endif
		retry++;
		if (ret < 0)
			mdelay(20);
	} while (ret < 0 && retry < I2C_ACCESS_MAX_RETRY);

	if (ret < 0)
		dev_err(info->dev, "%s: I2CW[0x%02X] = 0x%02X failed\n",
			__func__, cmd, data);
	else
		dev_dbg_ratelimited(info->dev, "%s: I2CW[0x%02X] = 0x%02X\n",
			__func__, cmd, data);

	return ret;
}

static int rt9465_i2c_write_byte(struct rt9465_info *info, u8 cmd, u8 data)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = _rt9465_i2c_write_byte(info, cmd, data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}

static inline int _rt9465_i2c_read_byte(struct rt9465_info *info, u8 cmd)
{
	int ret = 0, ret_val = 0, retry = 0;

	do {
#ifdef CONFIG_RT_REGMAP
		ret = rt_regmap_block_read(info->regmap_dev, cmd, 1, &ret_val);
#else
		ret = rt9465_device_read(info->i2c, cmd, 1, &ret_val);
#endif
		retry++;
		if (ret < 0)
			msleep(20);
	} while (ret < 0 && retry < I2C_ACCESS_MAX_RETRY);

	if (ret < 0) {
		dev_err(info->dev, "%s: I2CR[0x%02X] failed\n", __func__, cmd);
		return ret;
	}

	ret_val = ret_val & 0xFF;

	dev_dbg_ratelimited(info->dev, "%s: I2CR[0x%02X] = 0x%02X\n", __func__,
		cmd, ret_val);

	return ret_val;
}

static int rt9465_i2c_read_byte(struct rt9465_info *info, u8 cmd)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = _rt9465_i2c_read_byte(info, cmd);
	mutex_unlock(&info->i2c_access_lock);

	if (ret < 0)
		return ret;

	return (ret & 0xFF);
}

static inline int _rt9465_i2c_block_write(struct rt9465_info *info, u8 cmd,
	u32 leng, const u8 *data)
{
	int ret = 0;

#ifdef CONFIG_RT_REGMAP
	ret = rt_regmap_block_write(info->regmap_dev, cmd, leng, data);
#else
	ret = rt9465_device_write(info->i2c, cmd, leng, data);
#endif

	return ret;
}


static int rt9465_i2c_block_write(struct rt9465_info *info, u8 cmd, u32 leng,
	const u8 *data)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = _rt9465_i2c_block_write(info, cmd, leng, data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}

static inline int _rt9465_i2c_block_read(struct rt9465_info *info, u8 cmd,
	u32 leng, u8 *data)
{
	int ret = 0;

#ifdef CONFIG_RT_REGMAP
	ret = rt_regmap_block_read(info->regmap_dev, cmd, leng, data);
#else
	ret = rt9465_device_read(info->i2c, cmd, leng, data);
#endif

	return ret;
}

static int rt9465_i2c_block_read(struct rt9465_info *info, u8 cmd, u32 leng,
	u8 *data)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = _rt9465_i2c_block_read(info, cmd, leng, data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}

#if 0
static int rt9465_i2c_test_bit(struct rt9465_info *info, u8 cmd, u8 shift)
{
	int ret = 0;

	ret = rt9465_i2c_read_byte(info, cmd);
	if (ret < 0)
		return ret;

	ret = ret & (1 << shift);

	return ret;
}
#endif

static int rt9465_i2c_update_bits(struct rt9465_info *info, u8 cmd, u8 data,
	u8 mask)
{
	int ret = 0;
	u8 reg_data = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = _rt9465_i2c_read_byte(info, cmd);
	if (ret < 0) {
		mutex_unlock(&info->i2c_access_lock);
		return ret;
	}

	reg_data = ret & 0xFF;
	reg_data &= ~mask;
	reg_data |= (data & mask);

	ret = _rt9465_i2c_write_byte(info, cmd, reg_data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}

static inline int rt9465_set_bit(struct rt9465_info *info, u8 reg, u8 mask)
{
	return rt9465_i2c_update_bits(info, reg, mask, mask);
}

static inline int rt9465_clr_bit(struct rt9465_info *info, u8 reg, u8 mask)
{
	return rt9465_i2c_update_bits(info, reg, 0x00, mask);
}

/* ================== */
/* Internal Functions */
/* ================== */

/* The following APIs will be reference in internal functions */
static int rt9465_set_ichg(struct charger_device *chg_dev, u32 uA);
static int rt9465_get_ichg(struct charger_device *chg_dev, u32 *uA);
static int rt9465_set_mivr(struct charger_device *chg_dev, u32 uV);
static int rt9465_set_battery_voreg(struct charger_device *chg_dev, u32 uV);
static int rt9465_enable_safety_timer(struct charger_device *chg_dev, bool en);

static u8 rt9465_find_closest_reg_value(const u32 min, const u32 max,
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

static u32 rt9465_find_closest_real_value(const u32 min, const u32 max,
	const u32 step, const u8 reg_val)
{
	u32 ret_val = 0;

	ret_val = min + reg_val * step;
	if (ret_val > max)
		ret_val = max;

	return ret_val;
}

static int rt9465_enable_hidden_mode(struct rt9465_info *info, bool en)
{
	int ret = 0;

	dev_info(info->dev, "%s: enable = %d\n", __func__, en);

	/* Disable hidden mode */
	if (!en) {
		ret = rt9465_i2c_write_byte(info, 0x50, 0x00);
		if (ret < 0)
			goto err;
		return ret;
	}

	/* Enable hidden mode */
	ret = rt9465_i2c_block_write(info, rt9465_hidden_mode_addr[0],
		ARRAY_SIZE(rt9465_hidden_mode_val),
		rt9465_hidden_mode_val);
	if (ret < 0)
		goto err;

	return ret;

err:
	dev_err(info->dev, "%s: enable = %d failed, ret = %d\n", __func__, en, ret);
	return ret;
}
static int rt9465_sw_workaround(struct rt9465_info *info)
{
	int ret = 0;

	dev_info(info->dev, "%s\n", __func__);

	/* Enter hidden mode */
	ret = rt9465_enable_hidden_mode(info, true);
	if (ret < 0)
		return ret;

	/* For chip rev == E2, set Hidden code to make ICHG accurate */
	if (info->device_id == RT9465_VERSION_E2) {
		ret = rt9465_i2c_write_byte(info, RT9465_REG_HIDDEN_CTRL2, 0x68);
		if (ret < 0)
			goto out;
		ret = rt9465_i2c_write_byte(info, RT9465_REG_HIDDEN_CTRL6, 0x3A);
		if (ret < 0)
			goto out;
	}

out:
	rt9465_enable_hidden_mode(info, false);
	return ret;
}

static irqreturn_t rt9465_irq_handler(int irq, void *data)
{
	int ret = 0;
	u8 irq_data[4] = {0};
	struct rt9465_info *info = (struct rt9465_info *)data;

	dev_info(info->dev, "%s\n", __func__);

	ret = rt9465_i2c_block_read(info, RT9465_REG_CHG_STATC,
		ARRAY_SIZE(irq_data), irq_data);
	if (ret < 0) {
		dev_err(info->dev, "%s: read irq failed\n", __func__);
		goto err_read_irq;
	}

	dev_info(info->dev, "%s: STATC = 0x%02X, FAULT = 0x%02X\n",
		__func__, irq_data[0], irq_data[1]);
	dev_info(info->dev, "%s: IRQ1 = 0x%02X, IRQ2 = 0x%02X\n",
		__func__, irq_data[2], irq_data[3]);

err_read_irq:
	return IRQ_HANDLED;
}

static int rt9465_register_irq(struct rt9465_info *info)
{
	int ret = 0;
	struct device_node *np;

	/* Parse irq number from dts */
	np = of_find_node_by_name(NULL, info->desc->eint_name);
	if (np)
		info->irq = irq_of_parse_and_map(np, 0);
	else {
		dev_err(info->dev, "%s: cannot get node\n", __func__);
		ret = -ENODEV;
		goto err_nodev;
	}
	dev_info(info->dev, "%s: irq = %d\n", __func__, info->irq);

	/* Request threaded IRQ */
	ret = request_threaded_irq(info->irq, NULL, rt9465_irq_handler,
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT, info->desc->eint_name,
		info);
	if (ret < 0) {
		dev_err(info->dev, "%s: request thread irq failed\n", __func__);
		goto err_request_irq;
	}

	return 0;

err_nodev:
err_request_irq:
	return ret;
}

static int rt9465_enable_all_irq(struct rt9465_info *info, const bool enable)
{
	int ret = 0;
	u8 irq_data[4] = {0};
	u8 mask = enable ? 0x00 : 0xFF;

	memset(irq_data, mask, ARRAY_SIZE(irq_data));

	dev_info(info->dev, "%s: enable = %d\n", __func__, enable);

	/* Enable/Disable all irq */
	ret = rt9465_device_write(info->i2c, RT9465_REG_CHG_STATC_MASK,
		ARRAY_SIZE(irq_data), irq_data);
	if (ret < 0)
		dev_err(info->dev, "%s: %s irq failed\n", __func__,
			(enable ? "enable" : "disable"));

	return ret;
}

static int rt9465_irq_init(struct rt9465_info *info)
{
	int ret = 0;

	/* Enable all IRQs */
	ret = rt9465_enable_all_irq(info, true);
	if (ret < 0)
		dev_err(info->dev, "%s: enable all irq failed\n", __func__);
	return ret;
}

static bool rt9465_is_hw_exist(struct rt9465_info *info)
{
	int ret = 0;
	u8 version = 0;

	ret = i2c_smbus_read_byte_data(info->i2c, RT9465_REG_SYSTEM1);
	if (ret < 0) {
		dev_err(info->dev, "%s: failed, ret = %d\n", __func__, ret);
		return false;
	}

	version = (ret & RT9465_MASK_VERSION) >> RT9465_SHIFT_VERSION;
	dev_info(info->dev, "%s: E%d(0x%02X)\n", __func__, version + 1, version);

	info->device_id = version;
	return true;
}

static int rt9465_set_fast_charge_timer(struct rt9465_info *info,
	const u32 hour)
{
	int ret = 0;
	u8 reg_fct = 0;

	dev_info(info->dev, "%s: set fast charge timer to %d\n", __func__, hour);

	reg_fct = rt9465_find_closest_reg_value(RT9465_WT_FC_MIN, RT9465_WT_FC_MAX,
		RT9465_WT_FC_STEP, RT9465_WT_FC_NUM, hour);

	ret = rt9465_i2c_update_bits(
		info,
		RT9465_REG_CHG_CTRL9,
		reg_fct << RT9465_SHIFT_WT_FC,
		RT9465_MASK_WT_FC
	);

	return ret;
}

static int rt9465_enable_wdt(struct rt9465_info *info, bool enable)
{
	int ret = 0;

	dev_info(info->dev, "%s: enable = %d\n", __func__, enable);

	ret = (enable ? rt9465_set_bit : rt9465_clr_bit)
		(info, RT9465_REG_CHG_CTRL10, RT9465_MASK_WDT_EN);

	return ret;
}

static int rt9465_reset_chip(struct rt9465_info *info)
{
	int ret = 0;

	dev_info(info->dev, "%s\n", __func__);

	ret = rt9465_set_bit(info, RT9465_REG_CHG_CTRL0, RT9465_MASK_RST);

	return ret;
}

static int rt9465_enable_te(struct rt9465_info *info, const bool enable)
{
	int ret = 0;

	dev_info(info->dev, "%s: enable = %d\n", __func__, enable);

	ret = (enable ? rt9465_set_bit : rt9465_clr_bit)
		(info, RT9465_REG_CHG_CTRL8, RT9465_MASK_TE_EN);

	return ret;
}

static int rt9465_set_ieoc(struct rt9465_info *info, u32 ieoc)
{
	int ret = 0;
	u8 reg_ieoc = 0;

	/* Workaround for E1, IEOC must >= 700mA */
	if (ieoc < 700000 && info->device_id == RT9465_VERSION_E1)
		ieoc = 700000;

	/* Find corresponding reg value */
	reg_ieoc = rt9465_find_closest_reg_value(RT9465_IEOC_MIN,
		RT9465_IEOC_MAX, RT9465_IEOC_STEP, RT9465_IEOC_NUM, ieoc);
	reg_ieoc += 0x05;
	dev_info(info->dev, "%s: ieoc = %d\n", __func__, ieoc);

	ret = rt9465_i2c_update_bits(
		info,
		RT9465_REG_CHG_CTRL7,
		reg_ieoc << RT9465_SHIFT_IEOC,
		RT9465_MASK_IEOC
	);

	return ret;
}

static int rt9465_get_mivr(struct rt9465_info *info, u32 *mivr)
{
	int ret = 0;
	u8 reg_mivr = 0;

	ret = rt9465_i2c_read_byte(info, RT9465_REG_CHG_CTRL5);
	if (ret < 0)
		return ret;
	reg_mivr = ((ret & RT9465_MASK_MIVR) >> RT9465_SHIFT_MIVR) & 0xFF;

	*mivr = rt9465_find_closest_real_value(RT9465_MIVR_MIN, RT9465_MIVR_MAX,
		RT9465_MIVR_STEP, reg_mivr);


	return ret;
}

static int rt9465_init_setting(struct rt9465_info *info)
{
	int ret = 0;
	struct rt9465_desc *desc = info->desc;

	dev_info(info->dev, "%s\n", __func__);

	ret = rt9465_irq_init(info);
	if (ret < 0)
		dev_err(info->dev, "%s: init irq failed\n", __func__);

	ret = rt9465_set_ichg(info->chg_dev, desc->ichg);
	if (ret < 0)
		dev_err(info->dev, "%s: set ichg failed\n", __func__);

	ret = rt9465_set_mivr(info->chg_dev, desc->mivr);
	if (ret < 0)
		dev_err(info->dev, "%s: set mivr failed\n", __func__);

	ret = rt9465_set_ieoc(info, desc->ieoc);
	if (ret < 0)
		dev_err(info->dev, "%s: set ieoc failed\n", __func__);

	ret = rt9465_set_battery_voreg(info->chg_dev, desc->cv);
	if (ret < 0)
		dev_err(info->dev, "%s: set cv failed\n", __func__);

	ret = rt9465_enable_te(info, desc->enable_te);
	if (ret < 0)
		dev_err(info->dev, "%s: set te failed\n", __func__);

	/* Set fast charge timer to 12 hours */
	ret = rt9465_set_fast_charge_timer(info, desc->safety_timer);
	if (ret < 0)
		dev_err(info->dev, "%s: set fast timer failed\n", __func__);

	ret = rt9465_enable_safety_timer(info->chg_dev, true);
	if (ret < 0)
		dev_err(info->dev, "%s: enable charger timer failed\n", __func__);

	ret = rt9465_enable_wdt(info, desc->enable_wdt);
	if (ret < 0)
		dev_err(info->dev, "%s: enable watchdog failed\n", __func__);

	return ret;
}

static int rt9465_get_charging_status(struct rt9465_info *info,
	enum rt9465_charging_status *chg_stat)
{
	int ret = 0;

	ret = rt9465_i2c_read_byte(info, RT9465_REG_SYSTEM1);
	if (ret < 0)
		return ret;

	*chg_stat = (ret & RT9465_MASK_CHG_STAT) >> RT9465_SHIFT_CHG_STAT;

	return ret;
}

static int rt9465_get_ieoc(struct rt9465_info *info, u32 *ieoc)
{
	int ret = 0;
	u8 reg_ieoc = 0;

	ret = rt9465_i2c_read_byte(info, RT9465_REG_CHG_CTRL7);
	if (ret < 0)
		return ret;

	reg_ieoc = (ret & RT9465_MASK_IEOC) >> RT9465_SHIFT_IEOC;
	reg_ieoc -= 0x05;
	*ieoc = rt9465_find_closest_real_value(RT9465_IEOC_MIN, RT9465_IEOC_MAX,
		RT9465_IEOC_STEP, reg_ieoc);

	return ret;
}

static int rt9465_parse_dt(struct rt9465_info *info, struct device *dev)
{
	struct rt9465_desc *desc = NULL;
	struct device_node *np = dev->of_node;

	dev_info(info->dev, "%s\n", __func__);

	if (!np) {
		dev_err(info->dev, "%s: no device node\n", __func__);
		return -EINVAL;
	}

	info->desc = &rt9465_default_desc;

	desc = devm_kzalloc(dev, sizeof(struct rt9465_desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;
	memcpy(desc, &rt9465_default_desc, sizeof(struct rt9465_desc));

	/*
	 * The following is how gpio is uesed on MTK's platform, "GPIO pinctrl"
	 * Please modify it if you have your own method
	 */
	info->en_pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(info->en_pinctrl)) {
		dev_err(dev, "%s: cannot find en pinctrl!\n", __func__);
		info->en_pinctrl = NULL;
	}

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

#ifdef CONFIG_RT9465_PWR_EN_TO_MT6336
	info->lowq_ctrl = mt6336_ctrl_get("rt9465");

	mt6336_ctrl_enable(info->lowq_ctrl);
	mt6336_set_flag_register_value(MT6336_GPIO11_MODE, 0x0);
	mt6336_set_flag_register_value(MT6336_GPIO_DIR1_SET, 0x8);
	mt6336_ctrl_disable(info->lowq_ctrl);
#endif

	if (of_property_read_u32(np, "regmap_represent_slave_addr",
		&(desc->regmap_represent_slave_addr)) < 0)
		dev_err(info->dev, "%s: no regmap represent slave addr\n", __func__);

	if (of_property_read_string(np, "regmap_name", &desc->regmap_name) < 0)
		dev_err(info->dev, "%s: no regmap name\n", __func__);

	if (of_property_read_string(np, "alias_name",
		&info->chg_props.alias_name) < 0)
		dev_err(info->dev, "%s: no alias name\n", __func__);

	if (of_property_read_string(np, "eint_name", &desc->eint_name) < 0)
		dev_err(info->dev, "%s: no eint name\n", __func__);

	/*
	 * For dual charger, one is primary_chg;
	 * another one will be secondary_chg
	 */
	if (of_property_read_string(np, "charger_name",
		&desc->chg_dev_name) < 0)
		dev_err(info->dev, "%s: no charger name\n", __func__);

	if (of_property_read_u32(np, "ichg", &desc->ichg) < 0)
		dev_err(info->dev, "%s: no ichg\n", __func__);

	if (of_property_read_u32(np, "mivr", &desc->mivr) < 0)
		dev_err(info->dev, "%s: no mivr\n", __func__);

	if (of_property_read_u32(np, "cv", &desc->cv) < 0)
		dev_err(info->dev, "%s: no cv\n", __func__);

	if (of_property_read_u32(np, "ieoc", &desc->ieoc) < 0)
		dev_err(info->dev, "%s: no ieoc\n", __func__);

	if (of_property_read_u32(np, "safety_timer", &desc->safety_timer) < 0)
		dev_err(info->dev, "%s: no safety timer\n", __func__);

	desc->enable_te = of_property_read_bool(np, "enable_te");
	desc->enable_wdt = of_property_read_bool(np, "enable_wdt");

	info->desc = desc;

	return 0;
}

/* =========================================================== */
/* The following is implementation for interface of rt_charger */
/* =========================================================== */

static int rt9465_enable_chip(struct charger_device *chg_dev, bool enable)
{
	struct rt9465_info *info = dev_get_drvdata(&chg_dev->dev);

	dev_info(info->dev, "%s: enable = %d\n", __func__, enable);
#ifndef CONFIG_RT9465_PWR_EN_TO_MT6336
	if (!info->en_pinctrl || !info->en_enable || !info->en_disable) {
		dev_err(info->dev, "%s: no en pinctrl\n", __func__);
		return -EIO;
	}
#endif

	mutex_lock(&info->gpio_access_lock);
	if (enable) {
		i2c_lock_adapter(info->i2c->adapter);
#ifndef CONFIG_RT9465_PWR_EN_TO_MT6336
		pinctrl_select_state(info->en_pinctrl, info->en_enable);
#else
		mt6336_ctrl_enable(info->lowq_ctrl);
		mt6336_set_flag_register_value(MT6336_GPIO_DOUT1_SET, 0x8);
		mt6336_ctrl_disable(info->lowq_ctrl);
#endif
		dev_info(info->dev, "%s: set gpio high\n", __func__);
		udelay(10);
		i2c_unlock_adapter(info->i2c->adapter);
	} else {
#ifndef CONFIG_RT9465_PWR_EN_TO_MT6336
		pinctrl_select_state(info->en_pinctrl, info->en_disable);
#else
		mt6336_ctrl_enable(info->lowq_ctrl);
		mt6336_set_flag_register_value(MT6336_GPIO_DOUT0_CLR, 0x8);
		mt6336_ctrl_disable(info->lowq_ctrl);
#endif
		dev_info(info->dev, "%s: set gpio low\n", __func__);
	}

	/* Wait for chip's enable/disable */
	mdelay(1);
	mutex_unlock(&info->gpio_access_lock);

	return 0;
}

static int rt9465_is_charging_enabled(struct charger_device *chg_dev, bool *en)
{
	int ret = 0;
	struct rt9465_info *info = dev_get_drvdata(&chg_dev->dev);

	ret = rt9465_i2c_read_byte(info, RT9465_REG_CHG_CTRL1);
	if (ret < 0)
		return ret;

	*en = (((ret & RT9465_MASK_CHG_EN) >> RT9465_SHIFT_CHG_EN) & 0xFF)
		> 0 ? true : false;

	return ret;
}

static int rt9465_dump_register(struct charger_device *chg_dev)
{
	int i = 0, ret = 0;
	int ichg = 0;
	u32 mivr = 0, ieoc = 0;
	bool chg_enable = 0;
	enum rt9465_charging_status chg_status = RT9465_CHG_STATUS_READY;
	struct rt9465_info *info = dev_get_drvdata(&chg_dev->dev);

	ret = rt9465_get_ichg(chg_dev, &ichg);
	ret = rt9465_get_mivr(info, &mivr);
	ret = rt9465_is_charging_enabled(chg_dev, &chg_enable);
	ret = rt9465_get_ieoc(info, &ieoc);
	ret = rt9465_get_charging_status(info, &chg_status);

	for (i = 0; i < ARRAY_SIZE(rt9465_reg_addr); i++) {
		ret = rt9465_i2c_read_byte(info, rt9465_reg_addr[i]);
		if (ret < 0)
			return ret;
	}

	dev_info(info->dev, "%s: ICHG = %dmA, MIVR = %dmV, IEOC = %dmA\n",
		__func__, ichg / 1000, mivr / 1000, ieoc / 1000);

	dev_info(info->dev, "%s: CHG_EN = %d, CHG_STATUS = %s\n",
		__func__, chg_enable, rt9465_chg_status_name[chg_status]);

	return ret;
}

static int rt9465_enable_charging(struct charger_device *chg_dev, bool en)
{
	int ret = 0;
	struct rt9465_info *info = dev_get_drvdata(&chg_dev->dev);

	ret = (en ? rt9465_set_bit : rt9465_clr_bit)
		(info, RT9465_REG_CHG_CTRL1, RT9465_MASK_CHG_EN);

	return ret;
}

static int rt9465_enable_safety_timer(struct charger_device *chg_dev, bool en)
{
	int ret = 0;
	struct rt9465_info *info = dev_get_drvdata(&chg_dev->dev);

	ret = (en ? rt9465_set_bit : rt9465_clr_bit)
		(info, RT9465_REG_CHG_CTRL9, RT9465_MASK_TMR_EN);

	return ret;
}

static int rt9465_set_ichg(struct charger_device *chg_dev, u32 uA)
{
	int ret = 0;
	u8 reg_ichg = 0;
	struct rt9465_info *info = dev_get_drvdata(&chg_dev->dev);

	/* Workaround for E1, Ichg must >= 1000mA */
	if (uA < 1000000 && info->device_id == RT9465_VERSION_E1)
		uA = 1000000;

	/* Find corresponding reg value */
	reg_ichg = rt9465_find_closest_reg_value(RT9465_ICHG_MIN,
		RT9465_ICHG_MAX, RT9465_ICHG_STEP, RT9465_ICHG_NUM, uA);

	reg_ichg += 0x06;

	dev_info(info->dev, "%s: ichg = %d\n", __func__, uA);

	ret = rt9465_i2c_update_bits(
		info,
		RT9465_REG_CHG_CTRL6,
		reg_ichg << RT9465_SHIFT_ICHG,
		RT9465_MASK_ICHG
	);

	return ret;
}

static int rt9465_set_mivr(struct charger_device *chg_dev, u32 uV)
{
	int ret = 0;
	u8 reg_mivr = 0;
	struct rt9465_info *info = dev_get_drvdata(&chg_dev->dev);

	/* Find corresponding reg value */
	reg_mivr = rt9465_find_closest_reg_value(RT9465_MIVR_MIN,
		RT9465_MIVR_MAX, RT9465_MIVR_STEP, RT9465_MIVR_NUM, uV);

	dev_info(info->dev, "%s: mivr = %d\n", __func__, uV);

	ret = rt9465_i2c_update_bits(
		info,
		RT9465_REG_CHG_CTRL5,
		reg_mivr << RT9465_SHIFT_MIVR,
		RT9465_MASK_MIVR
	);

	return ret;
}

static int rt9465_set_battery_voreg(struct charger_device *chg_dev, u32 uV)
{
	int ret = 0;
	u8 reg_voreg = 0;
	u32 voreg = uV;
	struct rt9465_info *info = dev_get_drvdata(&chg_dev->dev);

	reg_voreg = rt9465_find_closest_reg_value(RT9465_BAT_VOREG_MIN,
		RT9465_BAT_VOREG_MAX, RT9465_BAT_VOREG_STEP,
		RT9465_BAT_VOREG_NUM, voreg);

	dev_info(info->dev, "%s: bat voreg = %d\n", __func__, voreg);

	ret = rt9465_i2c_update_bits(
		info,
		RT9465_REG_CHG_CTRL3,
		reg_voreg << RT9465_SHIFT_BAT_VOREG,
		RT9465_MASK_BAT_VOREG
	);

	return ret;
}


static int rt9465_get_ichg(struct charger_device *chg_dev, u32 *uA)
{
	int ret = 0;
	u8 reg_ichg = 0;
	struct rt9465_info *info = dev_get_drvdata(&chg_dev->dev);

	ret = rt9465_i2c_read_byte(info, RT9465_REG_CHG_CTRL6);
	if (ret < 0)
		return ret;

	reg_ichg = (ret & RT9465_MASK_ICHG) >> RT9465_SHIFT_ICHG;
	reg_ichg -= 0x06;
	*uA = rt9465_find_closest_real_value(RT9465_ICHG_MIN, RT9465_ICHG_MAX,
		RT9465_ICHG_STEP, reg_ichg);

	return ret;
}

static int rt9465_get_min_ichg(struct charger_device *chg_dev, u32 *uA)
{
	int ret = 0;

	*uA = rt9465_find_closest_real_value(RT9465_ICHG_MIN, RT9465_ICHG_MAX,
		RT9465_ICHG_STEP, 0);

	return ret;
}

static int rt9465_get_tchg(struct charger_device *chg_dev,
	int *tchg_min, int *tchg_max)
{
	int ret = 0, reg_adc_temp = 0, adc_temp = 0;
	struct rt9465_info *info = dev_get_drvdata(&chg_dev->dev);

	mutex_lock(&info->adc_access_lock);

	/* Get value from ADC */
	ret = rt9465_i2c_read_byte(info, RT9465_REG_CHG_CTRL12);
	if (ret < 0)
		goto out;

	reg_adc_temp = (ret & RT9465_MASK_ADC_RPT) >> RT9465_SHIFT_ADC_RPT;
	if (reg_adc_temp == 0x00) {
		*tchg_min = 0;
		*tchg_max = 60;
	} else {
		reg_adc_temp -= 0x01;
		adc_temp = rt9465_find_closest_real_value(RT9465_ADC_RPT_MIN,
			RT9465_ADC_RPT_MAX, RT9465_ADC_RPT_STEP, reg_adc_temp);
		*tchg_min = adc_temp + 1;
		*tchg_max = adc_temp + RT9465_ADC_RPT_STEP;
	}

	dev_info(info->dev, "%s: %d < temperature <= %d\n", __func__, *tchg_min, *tchg_max);

out:
	mutex_unlock(&info->adc_access_lock);
	return ret;
}

static int rt9465_is_charging_done(struct charger_device *chg_dev, bool *done)
{
	int ret = 0;
	enum rt9465_charging_status chg_stat = RT9465_CHG_STATUS_READY;
	struct rt9465_info *info = dev_get_drvdata(&chg_dev->dev);

	ret = rt9465_get_charging_status(info, &chg_stat);

	/* Return is charging done or not */
	switch (chg_stat) {
	case RT9465_CHG_STATUS_READY:
	case RT9465_CHG_STATUS_PROGRESS:
	case RT9465_CHG_STATUS_FAULT:
		*done = false;
		break;
	case RT9465_CHG_STATUS_DONE:
		*done = true;
		break;
	default:
		*done = false;
		break;
	}

	return ret;
}


static int rt9465_kick_wdt(struct charger_device *chg_dev)
{
	int ret = 0;
	struct rt9465_info *info = dev_get_drvdata(&chg_dev->dev);
	enum rt9465_charging_status chg_status = RT9465_CHG_STATUS_READY;

	/* Workaround: enable/disable watchdog to kick it */
	if (info->device_id == RT9465_VERSION_E1 ||
	    info->device_id == RT9465_VERSION_E2) {
		ret = rt9465_enable_wdt(info, false);
		if (ret < 0)
			dev_err(info->dev, "%s: disable wdt failed\n", __func__);

		ret = rt9465_enable_wdt(info, true);
		if (ret < 0)
			dev_err(info->dev, "%s: enable wdt failed\n", __func__);

		return ret;
	}

	/* Any I2C communication can kick wdt */
	ret = rt9465_get_charging_status(info, &chg_status);
	return ret;
}

static struct charger_ops rt9465_chg_ops = {
	.enable = rt9465_enable_charging,
	.is_enabled = rt9465_is_charging_enabled,
	.enable_safety_timer = rt9465_enable_safety_timer,
	.enable_chip = rt9465_enable_chip,
	.dump_registers = rt9465_dump_register,
	.is_charging_done = rt9465_is_charging_done,
	.get_charging_current = rt9465_get_ichg,
	.set_charging_current = rt9465_set_ichg,
	.get_min_charging_current = rt9465_get_min_ichg,
	.set_constant_voltage = rt9465_set_battery_voreg,
	.kick_wdt = rt9465_kick_wdt,
	.get_tchg_adc = rt9465_get_tchg,
	.set_mivr = rt9465_set_mivr,
};

/* ========================= */
/* I2C driver function       */
/* ========================= */

static int rt9465_probe(struct i2c_client *i2c,
	const struct i2c_device_id *dev_id)
{
	int ret = 0;
	struct rt9465_info *info = NULL;

	dev_info(info->dev, "%s (%s)\n", __func__, RT9465_DRV_VERSION);

	info = devm_kzalloc(&i2c->dev, sizeof(struct rt9465_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->i2c = i2c;
	info->dev = &i2c->dev;
	mutex_init(&info->i2c_access_lock);
	mutex_init(&info->adc_access_lock);
	mutex_init(&info->gpio_access_lock);

	/* Must parse en gpio and try to enable rt9465 first */
	ret = rt9465_parse_dt(info, &i2c->dev);
	if (ret < 0) {
		dev_err(info->dev, "%s: parse dt failed\n", __func__);
		goto err_parse_dt;
	}

	/* Register charger device */
	info->chg_dev = charger_device_register(info->desc->chg_dev_name,
		&i2c->dev, info, &rt9465_chg_ops, &info->chg_props);
	if (IS_ERR_OR_NULL(info->chg_dev)) {
		ret = PTR_ERR(info->chg_dev);
		goto err_register_chg_dev;
	}

	ret = rt9465_enable_chip(info->chg_dev, true);
	if (ret < 0) {
		dev_err(info->dev, "%s: enable chip failed\n", __func__);
		goto err_enable_chip;
	}

	/* Is HW exist */
	if (!rt9465_is_hw_exist(info)) {
		dev_err(info->dev, "%s: no rt9465 exists\n", __func__);
		return -ENODEV;
		goto err_no_dev;
	}
	i2c_set_clientdata(i2c, info);

	ret = rt9465_register_irq(info);
	if (ret < 0) {
		dev_err(info->dev, "%s: irq init failed\n", __func__);
		goto err_register_irq;
	}

#ifdef CONFIG_RT_REGMAP
	ret = rt9465_register_rt_regmap(info);
	if (ret < 0)
		goto err_register_regmap;
#endif
	/* Reset chip */
	ret = rt9465_reset_chip(info);
	if (ret < 0)
		dev_err(info->dev, "%s: set register to default value failed\n", __func__);

	ret = rt9465_init_setting(info);
	if (ret < 0) {
		dev_err(info->dev, "%s: set failed\n", __func__);
		goto err_init_setting;
	}

	ret = rt9465_sw_workaround(info);
	if (ret < 0) {
		dev_err(info->dev, "%s: sw workaround failed\n", __func__);
		goto err_sw_workaround;
	}

	rt9465_dump_register(info->chg_dev);

	dev_info(info->dev, "%s: ends\n", __func__);

	return ret;

err_sw_workaround:
err_init_setting:
#ifdef CONFIG_RT_REGMAP
	rt_regmap_device_unregister(info->regmap_dev);
err_register_regmap:
#endif
err_register_irq:
err_no_dev:
	rt9465_enable_chip(info->chg_dev, false);
err_enable_chip:
	charger_device_unregister(info->chg_dev);
err_register_chg_dev:
err_parse_dt:
	mutex_destroy(&info->adc_access_lock);
	mutex_destroy(&info->i2c_access_lock);
	mutex_destroy(&info->gpio_access_lock);
	return ret;
}


static int rt9465_remove(struct i2c_client *i2c)
{
	int ret = 0;
	struct rt9465_info *info = i2c_get_clientdata(i2c);

	dev_info(info->dev, "%s\n", __func__);

	if (info) {
		if (info->chg_dev)
			charger_device_unregister(info->chg_dev);
#ifdef CONFIG_RT_REGMAP
		rt_regmap_device_unregister(info->regmap_dev);
#endif
		mutex_destroy(&info->adc_access_lock);
		mutex_destroy(&info->i2c_access_lock);
		mutex_destroy(&info->gpio_access_lock);
	}
	return ret;
}

static void rt9465_shutdown(struct i2c_client *i2c)
{
	int ret = 0;
	struct rt9465_info *info = i2c_get_clientdata(i2c);

	dev_info(info->dev, "%s\n", __func__);

	if (info) {
		ret = rt9465_reset_chip(info);
		if (ret < 0)
			dev_err(info->dev, "%s: sw reset failed\n", __func__);
	}
}

static const struct i2c_device_id rt9465_i2c_id[] = {
	{"rt9465", 0},
	{}
};

#ifdef CONFIG_OF
static const struct of_device_id rt9465_of_match[] = {
	{ .compatible = "mediatek,slave_charger", },
	{},
};
#else /* Not define CONFIG_OF */

#define RT9465_BUSNUM 1

static struct i2c_board_info rt9465_i2c_board_info __initdata = {
	I2C_BOARD_INFO("rt9465", RT9465_SALVE_ADDR)
};
#endif /* CONFIG_OF */


static struct i2c_driver rt9465_i2c_driver = {
	.driver = {
		.name = "rt9465",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = rt9465_of_match,
#endif
	},
	.probe = rt9465_probe,
	.remove = rt9465_remove,
	.shutdown = rt9465_shutdown,
	.id_table = rt9465_i2c_id,
};

static int __init rt9465_init(void)
{
	int ret = 0;

#ifdef CONFIG_OF
	pr_info("%s: with dts\n", __func__);
#else
	pr_info("%s: without dts\n", __func__);
	i2c_register_board_info(RT9465_BUSNUM, &rt9465_i2c_board_info, 1);
#endif

	ret = i2c_add_driver(&rt9465_i2c_driver);
	if (ret < 0)
		pr_err("%s: register i2c driver failed\n", __func__);

	return ret;
}
module_init(rt9465_init);


static void __exit rt9465_exit(void)
{
	i2c_del_driver(&rt9465_i2c_driver);
}
module_exit(rt9465_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("ShuFanLee <shufan_lee@richtek.com>");
MODULE_DESCRIPTION("RT9465 Charger Driver");
MODULE_VERSION(RT9465_DRV_VERSION);


/*
 * Version Note
 * 1.0.6
 * (1) Modify the way to kick WDT and the name of enable_watchdog_timer to
 *     enable_wdt
 * (2) Change pr_xxx to dev_xxx
 *
 * 1.0.5
 * (1) Modify charger name to secondary_chg
 *
 * 1.0.4
 * (1) Modify some pr_debug to pr_debug_ratelimited
 * (2) Modify the way to parse dt
 *
 * 1.0.3
 * (1) Modify rt9465_is_hw_exist to support all version
 * (2) Correct chip version
 * (3) Release rt9465_is_charging_enabled/rt9465_get_min_ichg
 *
 * 1.0.2
 * (1) Add ICHG accuracy workaround for E2
 * (2) Support E3 chip
 * (3) Add config to separate EN pin from MT6336 or AP
 *
 * 1.0.1
 * (1) Remove registering power supply class
 *
 * 1.0.0
 * Initial Release
 */
