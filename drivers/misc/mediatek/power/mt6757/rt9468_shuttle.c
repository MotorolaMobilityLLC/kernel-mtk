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
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <mt-plat/charging.h>
#include <mt-plat/battery_common.h>
#include <mt-plat/mtk_boot_common.h>
#include <mach/mtk_pe.h>
#include "rt9468_shuttle.h"
#include "mtk_charger_intf.h"

#ifdef CONFIG_RT_REGMAP
#include <mt-plat/rt-regmap.h>
#endif

#define I2C_ACCESS_MAX_RETRY	5

/* =============== */
/* RT9468 Variable */
/* =============== */

static const u32 rt9468_boost_oc_threshold[] = {
	500, 700, 1100, 1300, 1800, 2100, 2400,
}; /* mA */

enum rt9468_charging_status {
	RT9468_CHG_STATUS_READY = 0,
	RT9468_CHG_STATUS_PROGRESS,
	RT9468_CHG_STATUS_DONE,
	RT9468_CHG_STATUS_FAULT,
	RT9468_CHG_STATUS_MAX,
};

/* Charging status name */
static const char *rt9468_chg_status_name[RT9468_CHG_STATUS_MAX] = {
	"ready", "progress", "done", "fault",
};

static const unsigned char rt9468_reg_en_hidden_mode[] = {
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
};

static const unsigned char rt9468_val_en_hidden_mode[] = {
	0x49, 0x32, 0xB6, 0x27, 0x48, 0x18, 0x03, 0xE2,
};

enum rt9468_iin_limit_sel {
	RT9468_IIMLMTSEL_CHR_TYPE = 1,
	RT9468_IINLMTSEL_AICR = 2,
	RT9468_IINLMTSEL_LOWER_LEVEL, /* lower of above two */
};

enum rt9468_adc_sel {
	RT9468_ADC_VBUS_DIV5 = 1,
	RT9468_ADC_VBUS_DIV2,
	RT9468_ADC_VSYS,
	RT9468_ADC_VBAT,
	RT9468_ADC_VBATS,
	RT9468_ADC_TS_BAT,
	RT9468_ADC_TS_BUS,
	RT9468_ADC_IBUS,
	RT9468_ADC_IBAT,
	RT9468_ADC_IBATS,
	RT9468_ADC_VDDP,
	RT9468_ADC_TEMP_JC,
	RT9468_ADC_MAX,
};

/* Unit for each ADC parameter
 * 0 stands for reserved
 * For TS_BAT/TS_BUS, the real unit is 0.25.
 * Here we use 25, please remember to divide 100 while showing the value
 */
static const int rt9468_adc_unit[RT9468_ADC_MAX] = {
	0,
	RT9468_ADC_UNIT_VBUS_DIV5,
	RT9468_ADC_UNIT_VBUS_DIV2,
	RT9468_ADC_UNIT_VSYS,
	RT9468_ADC_UNIT_VBAT,
	RT9468_ADC_UNIT_VBATS,
	RT9468_ADC_UNIT_TS_BAT,
	RT9468_ADC_UNIT_TS_BUS,
	RT9468_ADC_UNIT_IBUS,
	RT9468_ADC_UNIT_IBAT,
	RT9468_ADC_UNIT_IBATS,
	RT9468_ADC_UNIT_REGN,
	RT9468_ADC_UNIT_TEMP_JC,
};

static const int rt9468_adc_offset[RT9468_ADC_MAX] = {
	0,
	RT9468_ADC_OFFSET_VBUS_DIV5,
	RT9468_ADC_OFFSET_VBUS_DIV2,
	RT9468_ADC_OFFSET_VSYS,
	RT9468_ADC_OFFSET_VBAT,
	RT9468_ADC_OFFSET_VBATS,
	RT9468_ADC_OFFSET_TS_BAT,
	RT9468_ADC_OFFSET_TS_BUS,
	RT9468_ADC_OFFSET_IBUS,
	RT9468_ADC_OFFSET_IBAT,
	RT9468_ADC_OFFSET_IBATS,
	RT9468_ADC_OFFSET_REGN,
	RT9468_ADC_OFFSET_TEMP_JC,
};


/* ======================= */
/* RT9468 Register Address */
/* ======================= */

static const unsigned char rt9468_reg_addr[] = {
	RT9468_REG_CORE_CTRL0,
	RT9468_REG_CHG_CTRL1,
	RT9468_REG_CHG_CTRL2,
	RT9468_REG_CHG_CTRL3,
	RT9468_REG_CHG_CTRL4,
	RT9468_REG_CHG_CTRL5,
	RT9468_REG_CHG_CTRL6,
	RT9468_REG_CHG_CTRL7,
	RT9468_REG_CHG_CTRL8,
	RT9468_REG_CHG_CTRL9,
	RT9468_REG_CHG_CTRL10,
	RT9468_REG_CHG_CTRL11,
	RT9468_REG_CHG_CTRL12,
	RT9468_REG_CHG_CTRL13,
	RT9468_REG_CHG_CTRL14,
	RT9468_REG_CHG_CTRL15,
	RT9468_REG_CHG_CTRL16,
	RT9468_REG_CHG_ADC,
	RT9468_REG_CHG_DPDM1,
	RT9468_REG_CHG_DPDM2,
	RT9468_REG_CHG_DPDM3,
	RT9468_REG_CHG_DPDM4,
	RT9468_REG_CHG_DPDM5,
	RT9468_REG_CHG_PUMP,
	RT9468_REG_DEVICE_ID,
	RT9468_REG_CHG_STAT,
	RT9468_REG_CHG_NTC,
	RT9468_REG_ADC_DATA_H,
	RT9468_REG_ADC_DATA_L,
	RT9468_REG_CHG_STATC,
	RT9468_REG_CHG_FAULT,
	RT9468_REG_TS_STATC,
	RT9468_REG_CHG_IRQ1,
	RT9468_REG_CHG_IRQ2,
	RT9468_REG_CHG_IRQ3,
	RT9468_REG_DPDM_IRQ,
	RT9468_REG_CHG_STATC_CTRL,
	RT9468_REG_CHG_FAULT_CTRL,
	RT9468_REG_TS_STATC_CTRL,
	RT9468_REG_CHG_IRQ1_CTRL,
	RT9468_REG_CHG_IRQ2_CTRL,
	RT9468_REG_CHG_IRQ3_CTRL,
	RT9468_REG_DPDM_IRQ_CTRL,
};


/* ================ */
/* RT9468 RT Regmap */
/* ================ */

#ifdef CONFIG_RT_REGMAP
RT_REG_DECL(RT9468_REG_CORE_CTRL0, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_CTRL1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_CTRL2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_CTRL3, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_CTRL4, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_CTRL5, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_CTRL6, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_CTRL7, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_CTRL8, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_CTRL9, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_CTRL10, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_CTRL11, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_CTRL12, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_CTRL13, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_CTRL14, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_CTRL15, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_CTRL16, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_ADC, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_DPDM1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_DPDM2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_DPDM3, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_DPDM4, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_DPDM5, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_PUMP, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_DEVICE_ID, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_STAT, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_NTC, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_ADC_DATA_H, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_ADC_DATA_L, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_STATC, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_FAULT, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_TS_STATC, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_IRQ1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_IRQ2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_IRQ3, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_DPDM_IRQ, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_STATC_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_FAULT_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_TS_STATC_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_IRQ1_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_IRQ2_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_CHG_IRQ3_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9468_REG_DPDM_IRQ_CTRL, 1, RT_VOLATILE, {});

static rt_register_map_t rt9468_regmap_map[] = {
	RT_REG(RT9468_REG_CORE_CTRL0),
	RT_REG(RT9468_REG_CHG_CTRL1),
	RT_REG(RT9468_REG_CHG_CTRL2),
	RT_REG(RT9468_REG_CHG_CTRL3),
	RT_REG(RT9468_REG_CHG_CTRL4),
	RT_REG(RT9468_REG_CHG_CTRL5),
	RT_REG(RT9468_REG_CHG_CTRL6),
	RT_REG(RT9468_REG_CHG_CTRL7),
	RT_REG(RT9468_REG_CHG_CTRL8),
	RT_REG(RT9468_REG_CHG_CTRL9),
	RT_REG(RT9468_REG_CHG_CTRL10),
	RT_REG(RT9468_REG_CHG_CTRL11),
	RT_REG(RT9468_REG_CHG_CTRL12),
	RT_REG(RT9468_REG_CHG_CTRL13),
	RT_REG(RT9468_REG_CHG_CTRL14),
	RT_REG(RT9468_REG_CHG_CTRL15),
	RT_REG(RT9468_REG_CHG_CTRL16),
	RT_REG(RT9468_REG_CHG_ADC),
	RT_REG(RT9468_REG_CHG_DPDM1),
	RT_REG(RT9468_REG_CHG_DPDM2),
	RT_REG(RT9468_REG_CHG_DPDM3),
	RT_REG(RT9468_REG_CHG_DPDM4),
	RT_REG(RT9468_REG_CHG_DPDM5),
	RT_REG(RT9468_REG_CHG_PUMP),
	RT_REG(RT9468_REG_DEVICE_ID),
	RT_REG(RT9468_REG_CHG_STAT),
	RT_REG(RT9468_REG_CHG_NTC),
	RT_REG(RT9468_REG_ADC_DATA_H),
	RT_REG(RT9468_REG_ADC_DATA_L),
	RT_REG(RT9468_REG_CHG_STATC),
	RT_REG(RT9468_REG_CHG_FAULT),
	RT_REG(RT9468_REG_TS_STATC),
	RT_REG(RT9468_REG_CHG_IRQ1),
	RT_REG(RT9468_REG_CHG_IRQ2),
	RT_REG(RT9468_REG_CHG_IRQ3),
	RT_REG(RT9468_REG_DPDM_IRQ),
	RT_REG(RT9468_REG_CHG_STATC_CTRL),
	RT_REG(RT9468_REG_CHG_FAULT_CTRL),
	RT_REG(RT9468_REG_TS_STATC_CTRL),
	RT_REG(RT9468_REG_CHG_IRQ1_CTRL),
	RT_REG(RT9468_REG_CHG_IRQ2_CTRL),
	RT_REG(RT9468_REG_CHG_IRQ3_CTRL),
	RT_REG(RT9468_REG_DPDM_IRQ_CTRL),
};
#endif /* CONFIG_RT_REGMAP */

struct rt9468_desc {
	u32 ichg;
	u32 aicr;
	u32 mivr;
	u32 cv;
	u32 ieoc;
	u32 safety_timer;
	bool enable_te;
	bool enable_wdt;
	int regmap_represent_slave_addr;
	const char *regmap_name;
};

/* These default values will be used if there's no property in dts */
static struct rt9468_desc rt9468_default_desc = {
	.ichg = 2000,
	.aicr = 500,
	.mivr = 4400,
	.cv = 4350,
	.ieoc = 250,
	.safety_timer = 12,
	.enable_te = true,
	.enable_wdt = true,
	.regmap_name = "rt9468",
	.regmap_represent_slave_addr = 0x5C,
};

struct rt9468_info {
	struct mtk_charger_info mchr_info;
	struct i2c_client *i2c;
	struct mutex i2c_access_lock;
	struct mutex adc_access_lock;
	struct work_struct otg_ctrl_work;
	struct workqueue_struct *otg_ctrl_workqueue;
	bool to_enable_otg;
	int i2c_log_level;
	int irq;
	struct rt9468_desc *desc;
	bool err_state;
#ifdef CONFIG_RT_REGMAP
	struct rt_regmap_device *regmap_dev;
	struct rt_regmap_properties *regmap_prop;
#endif
};

/* ========================= */
/* I2C operations            */
/* ========================= */

static int rt9468_device_read(void *client, u32 addr, int leng, void *dst)
{
	int ret = 0;
	struct i2c_client *i2c = NULL;

	i2c = (struct i2c_client *)client;
	ret = i2c_smbus_read_i2c_block_data(i2c, addr, leng, dst);

	return ret;
}

static int rt9468_device_write(void *client, u32 addr, int leng,
	const void *src)
{
	int ret = 0;
	struct i2c_client *i2c = NULL;

	i2c = (struct i2c_client *)client;
	ret = i2c_smbus_write_i2c_block_data(i2c, addr, leng, src);

	return ret;
}

#ifdef CONFIG_RT_REGMAP
static struct rt_regmap_fops rt9468_regmap_fops = {
	.read_device = rt9468_device_read,
	.write_device = rt9468_device_write,
};

static int rt9468_register_rt_regmap(struct rt9468_info *info)
{
	int ret = 0;
	struct i2c_client *i2c = info->i2c;
	struct rt_regmap_properties *prop = NULL;

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	prop = devm_kzalloc(&i2c->dev,
		sizeof(struct rt_regmap_properties), GFP_KERNEL);

	prop->name = info->desc->regmap_name;
	prop->aliases = info->desc->regmap_name;
	prop->register_num = ARRAY_SIZE(rt9468_regmap_map);
	prop->rm = rt9468_regmap_map;
	prop->rt_regmap_mode = RT_SINGLE_BYTE | RT_CACHE_DISABLE | RT_IO_PASS_THROUGH;
	prop->io_log_en = 0;

	info->regmap_prop = prop;
	info->regmap_dev = rt_regmap_device_register(
		info->regmap_prop,
		&rt9468_regmap_fops,
		&i2c->dev,
		i2c,
		info
	);

	if (!info->regmap_dev) {
		dev_err(&i2c->dev, "register regmap device failed\n");
		return -EINVAL;
	}

	battery_log(BAT_LOG_CRTI, "%s: ends\n", __func__);

	return ret;
}
#endif /* CONFIG_RT_REGMAP */

static inline int _rt9468_i2c_write_byte(struct rt9468_info *info, u8 cmd,
	u8 data)
{
	int ret = 0, retry = 0;

	do {
#ifdef CONFIG_RT_REGMAP
		ret = rt_regmap_block_write(info->regmap_dev, cmd, 1, &data);
#else
		ret = rt9468_device_write(info->i2c, cmd, 1, &data);
#endif
		retry++;
		if (ret < 0)
			mdelay(20);
	} while (ret < 0 && retry < I2C_ACCESS_MAX_RETRY);

	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: I2CW[0x%02X] = 0x%02X failed\n",
			__func__, cmd, data);
	else
		battery_log(info->i2c_log_level, "%s: I2CW[0x%02X] = 0x%02X\n",
			__func__, cmd, data);

	return ret;
}

static int rt9468_i2c_write_byte(struct rt9468_info *info, u8 cmd, u8 data)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = _rt9468_i2c_write_byte(info, cmd, data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}

static inline int _rt9468_i2c_read_byte(struct rt9468_info *info, u8 cmd)
{
	int ret = 0, ret_val = 0, retry = 0;

	do {
#ifdef CONFIG_RT_REGMAP
		ret = rt_regmap_block_read(info->regmap_dev, cmd, 1,
			&ret_val);
#else
		ret = rt9468_device_read(info->i2c, cmd, 1, &ret_val);
#endif
		retry++;
		if (ret < 0)
			mdelay(20);
	} while (ret < 0 && retry < I2C_ACCESS_MAX_RETRY);

	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: I2CR[0x%02X] failed\n",
			__func__, cmd);
		return ret;
	}

	ret_val = ret_val & 0xFF;

	battery_log(info->i2c_log_level, "%s: I2CR[0x%02X] = 0x%02X\n",
		__func__, cmd, ret_val);

	return ret_val;
}

static int rt9468_i2c_read_byte(struct rt9468_info *info, u8 cmd)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = _rt9468_i2c_read_byte(info, cmd);
	mutex_unlock(&info->i2c_access_lock);

	if (ret < 0)
		return ret;

	return (ret & 0xFF);
}

static inline int _rt9468_i2c_block_write(struct rt9468_info *info, u8 cmd,
	u32 leng, const u8 *data)
{
	int ret = 0;

#ifdef CONFIG_RT_REGMAP
	ret = rt_regmap_block_write(info->regmap_dev, cmd, leng, data);
#else
	ret = rt9468_device_write(info->i2c, cmd, leng, data);
#endif

	return ret;
}


static int rt9468_i2c_block_write(struct rt9468_info *info, u8 cmd, u32 leng,
	const u8 *data)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = rt9468_device_write(info->i2c, cmd, leng, data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}

static inline int _rt9468_i2c_block_read(struct rt9468_info *info, u8 cmd,
	u32 leng, u8 *data)
{
	int ret = 0;

#ifdef CONFIG_RT_REGMAP
	ret = rt_regmap_block_read(info->regmap_dev, cmd, leng, data);
#else
	ret = rt9468_device_read(info->i2c, cmd, leng, data);
#endif
	return ret;
}


static int rt9468_i2c_block_read(struct rt9468_info *info, u8 cmd, u32 leng,
	u8 *data)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = _rt9468_i2c_block_read(info, cmd, leng, data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}

static int rt9468_i2c_test_bit(struct rt9468_info *info, u8 cmd, u8 shift)
{
	int ret = 0;

	ret = rt9468_i2c_read_byte(info, cmd);
	if (ret < 0)
		return ret;

	ret = ret & (1 << shift);

	return ret;
}

static int rt9468_i2c_update_bits(struct rt9468_info *info, u8 cmd, u8 data,
	u8 mask)
{
	int ret = 0;
	u8 reg_data = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = _rt9468_i2c_read_byte(info, cmd);
	if (ret < 0) {
		mutex_unlock(&info->i2c_access_lock);
		return ret;
	}

	reg_data = ret & 0xFF;
	reg_data &= ~mask;
	reg_data |= (data & mask);

	ret = _rt9468_i2c_write_byte(info, cmd, reg_data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}

static inline int rt9468_set_bit(struct rt9468_info *info, u8 reg, u8 mask)
{
	return rt9468_i2c_update_bits(info, reg, mask, mask);
}

static inline int rt9468_clr_bit(struct rt9468_info *info, u8 reg, u8 mask)
{
	return rt9468_i2c_update_bits(info, reg, 0x00, mask);
}

/* ================== */
/* Internal Functions */
/* ================== */

/* The following APIs will be reference in internal functions */
static int rt_charger_set_ichg(struct mtk_charger_info *mchr_info, void *data);
static int rt_charger_set_aicr(struct mtk_charger_info *mchr_info, void *data);
static int rt_charger_set_mivr(struct mtk_charger_info *mchr_info, void *data);
static int rt_charger_get_ichg(struct mtk_charger_info *mchr_info, void *data);
static int rt_charger_get_aicr(struct mtk_charger_info *mchr_info, void *data);
static int rt_charger_set_boost_current_limit(
	struct mtk_charger_info *mchr_info, void *data);
static int rt_charger_enable_safety_timer(struct mtk_charger_info *mchr_info,
	void *data);
static int rt_charger_set_battery_voreg(struct mtk_charger_info *mchr_info,
	void *data);
static int rt_charger_is_power_path_enable(struct mtk_charger_info *mchr_info,
	void *data);

static u8 rt9468_find_closest_reg_value(const u32 min, const u32 max,
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

static u8 rt9468_find_closest_reg_value_via_table(const u32 *value_table,
	const u32 table_size, const u32 target_value)
{
	u32 i = 0;

	/* Smaller than minimum supported value, use minimum one */
	if (target_value < value_table[0])
		return 0;

	for (i = 0; i < table_size - 1; i++) {
		if (target_value >= value_table[i] && target_value < value_table[i + 1])
			return i;
	}

	/* Greater than maximum supported value, use maximum one */
	return table_size - 1;
}

static u32 rt9468_find_closest_real_value(const u32 min, const u32 max,
	const u32 step, const u8 reg_val)
{
	u32 ret_val = 0;

	ret_val = min + reg_val * step;
	if (ret_val > max)
		ret_val = max;

	return ret_val;
}

static irqreturn_t rt9468_irq_handler(int irq, void *data)
{
	int i = 0, j = 0, ret = 0;
	u8 irq_data[7] = {0};
	struct rt9468_info *info = (struct rt9468_info *)data;

	ret = rt9468_i2c_block_read(info, RT9468_REG_CHG_STATC, 7, irq_data);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: read irq failed\n", __func__);
		goto err_read_irq;
	}

	for (i = RT9468_REG_CHG_STATC; i <= RT9468_REG_DPDM_IRQ; i++, j++) {
		if (!irq_data[j])
			continue;
		battery_log(BAT_LOG_CRTI, "%s: reg[0x%02X] = 0x%02X\n",
			__func__, i, irq_data[j]);
	}

err_read_irq:
	return IRQ_HANDLED;
}

static int rt9468_irq_register(struct rt9468_info *info)
{
	int ret = 0;
	struct device_node *np;

	/* Parse irq number from dts */
	np = of_find_node_by_name(NULL, "chr_stat");
	if (np)
		info->irq = irq_of_parse_and_map(np, 0);
	else {
		battery_log(BAT_LOG_CRTI, "%s: cannot get node\n", __func__);
		ret = -ENODEV;
		goto err_nodev;
	}
	battery_log(BAT_LOG_CRTI, "%s: irq = %d\n", __func__, info->irq);

	/* Request threaded IRQ */
	ret = request_threaded_irq(info->irq, NULL, rt9468_irq_handler,
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "rt9468_irq", info);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: request thread irq failed\n",
			__func__);
		goto err_request_irq;
	}

	return 0;

err_nodev:
err_request_irq:
	return ret;
}

static int rt9468_enable_all_irq(struct rt9468_info *info, bool enable)
{
	int ret = 0;
	u8 irq_data[7] = {0};
	u8 mask = enable ? 0x00 : 0xFF;

	memset(irq_data, mask, ARRAY_SIZE(irq_data));

	/*
	 * Since this function will be used during shutdown,
	 * call original i2c api
	 * mask all irq
	 */
	ret = rt9468_device_write(info->i2c, RT9468_REG_CHG_STATC_CTRL,
		ARRAY_SIZE(irq_data), irq_data);

	return ret;
}

static int rt9468_irq_init(struct rt9468_info *info)
{
	int ret = 0;

	/* Enable all IRQ */
	ret = rt9468_enable_all_irq(info, false);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: enable all irq failed\n",
			__func__);

	return ret;
}

static bool rt9468_is_hw_exist(struct rt9468_info *info)
{
	int ret = 0;
	u8 revision = 0;

	ret = i2c_smbus_read_byte_data(info->i2c, RT9468_REG_DEVICE_ID);
	if (ret < 0)
		return false;

	revision = ret & 0xFF;
	if (revision == RT9468_DEVICE_ID_E1)
		battery_log(BAT_LOG_CRTI, "%s: rt9468 E1 revision\n", __func__);
	else
		return false;

	info->mchr_info.device_id = revision;
	return true;
}

static int rt9468_set_fast_charge_timer(struct rt9468_info *info, u32 hour)
{
	int ret = 0;
	u8 reg_fct = 0;

	battery_log(BAT_LOG_CRTI, "%s: set fast charge timer to %d\n",
		__func__, hour);

	reg_fct = rt9468_find_closest_reg_value(RT9468_WT_FC_MIN,
		RT9468_WT_FC_MAX, RT9468_WT_FC_STEP, RT9468_WT_FC_NUM, hour);

	ret = rt9468_i2c_update_bits(
		info,
		RT9468_REG_CHG_CTRL12,
		reg_fct << RT9468_SHIFT_WT_FC,
		RT9468_MASK_WT_FC
	);

	return ret;
}


/* Hardware pin current limit */
static int rt9468_enable_ilim(struct rt9468_info *info, bool enable)
{
	int ret = 0;

	battery_log(BAT_LOG_CRTI, "%s: enable ilim = %d\n", __func__, enable);

	ret = (enable ? rt9468_set_bit : rt9468_clr_bit)
		(info, RT9468_REG_CHG_CTRL3, RT9468_MASK_ILIM_EN);

	return ret;
}

/* Select IINLMTSEL to use AICR */
static int rt9468_select_input_current_limit(struct rt9468_info *info,
	enum rt9468_iin_limit_sel sel)
{
	int ret = 0;

	battery_log(BAT_LOG_CRTI, "%s: select input current limit = %d\n",
		__func__, sel);

	ret = rt9468_i2c_update_bits(
		info,
		RT9468_REG_CHG_CTRL2,
		sel << RT9468_SHIFT_IINLMTSEL,
		RT9468_MASK_IINLMTSEL
	);

	return ret;
}

/* Enter hidden mode */
static int rt9468_enable_hidden_mode(struct rt9468_info *info, bool enable)
{
	int ret = 0;

	battery_log(BAT_LOG_CRTI, "%s: enable hidden mode = %d\n",
		__func__, enable);

	/* Disable hidden mode */
	if (!enable) {
		ret = rt9468_i2c_write_byte(info, 0x70, 0x00);
		if (ret < 0)
			goto _err;
		return ret;
	}

	/* Enable hidden mode */
	ret = rt9468_i2c_block_write(
		info,
		rt9468_reg_en_hidden_mode[0],
		ARRAY_SIZE(rt9468_val_en_hidden_mode),
		rt9468_val_en_hidden_mode
	);
	if (ret < 0)
		goto _err;

	return ret;

_err:
	battery_log(BAT_LOG_CRTI, "%s: enable hidden mode = %d failed\n",
		__func__, enable);
	return ret;
}

static int rt9468_set_iprec(struct rt9468_info *info, u32 iprec)
{
	int ret = 0;
	u8 reg_iprec = 0;

	/* Find corresponding reg value */
	reg_iprec = rt9468_find_closest_reg_value(RT9468_IPREC_MIN,
		RT9468_IPREC_MAX, RT9468_IPREC_STEP, RT9468_IPREC_NUM, iprec);

	battery_log(BAT_LOG_CRTI, "%s: iprec = %d\n", __func__, iprec);

	ret = rt9468_i2c_update_bits(
		info,
		RT9468_REG_CHG_CTRL8,
		reg_iprec << RT9468_SHIFT_IPREC,
		RT9468_MASK_IPREC
	);

	return ret;
}


/* Software workaround */
static int rt9468_sw_workaround(struct rt9468_info *info)
{
	int ret = 0;
	u32 boot_mode = 0;

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	/* Enter hidden mode */
	ret = rt9468_enable_hidden_mode(info, true);
	if (ret < 0)
		goto _out;

	/* Set precharge current to 850mA, only do this in normal boot */
	mtk_charger_get_platform_boot_mode(&info->mchr_info, &boot_mode);
	if (boot_mode == NORMAL_BOOT) {
		ret = rt9468_set_iprec(info, 850);
		if (ret < 0)
			goto _out;
	}

	/* Increase Isys drop threshold to 2.5A */
	ret = rt9468_i2c_write_byte(info, 0x26, 0x1C);
	if (ret < 0)
		goto _out;

	/* Disable TS auto sensing */
	ret = rt9468_clr_bit(info, 0x2E, 0x01);
	if (ret < 0)
		goto _out;

	/* ICC: modify sensing node, make it more accurate */
	ret = rt9468_i2c_write_byte(info, 0x27, 0x00);
	if (ret < 0)
		goto _out;

	/* DIMIN level */
	ret = rt9468_i2c_write_byte(info, 0x28, 0x86);

_out:
	/* Exit hidden mode */
	ret = rt9468_enable_hidden_mode(info, false);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: exit hidden mode failed\n",
			__func__);

	return ret;
}

static int rt9468_get_adc(struct rt9468_info *info, enum rt9468_adc_sel adc_sel,
	int *adc_val)
{
	int ret = 0, i = 0;
	const int max_wait_retry = 5;
	u8 adc_data[2] = {0, 0};

	mutex_lock(&info->adc_access_lock);
	info->i2c_log_level = BAT_LOG_CRTI;

	/* Select ADC to desired channel */
	ret = rt9468_i2c_update_bits(
		info,
		RT9468_REG_CHG_ADC,
		adc_sel << RT9468_SHIFT_ADC_IN_SEL,
		RT9468_MASK_ADC_IN_SEL
	);

	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: select ADC to %d failed\n",
			__func__, adc_sel);
		goto _out;
	}

	/* Start ADC conversation */
	ret = rt9468_set_bit(info, RT9468_REG_CHG_ADC, RT9468_MASK_ADC_START);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI,
			"%s: start ADC conversation failed, sel = %d\n",
			__func__, adc_sel);
		goto _out;
	}

	battery_log(BAT_LOG_CRTI, "%s: adc set start bit, sel = %d\n",
		__func__, adc_sel);

	/* Wait for ADC conversation */
	for (i = 0; i < max_wait_retry; i++) {
		mdelay(35);
		if (rt9468_i2c_test_bit(info, RT9468_REG_CHG_ADC,
			RT9468_SHIFT_ADC_START) == 0) {
			battery_log(BAT_LOG_FULL,
				"%s: sel = %d, wait_times = %d\n",
				__func__, adc_sel, i);
			break;
		}
	}

	if (i == max_wait_retry) {
		battery_log(BAT_LOG_CRTI,
			"%s: Wait ADC conversation failed, sel = %d\n",
			__func__, adc_sel);
		ret = -EINVAL;
		goto _out;
	}

	mdelay(1);

	/* Read ADC data high/low byte */
	ret = rt9468_i2c_block_read(info, RT9468_REG_ADC_DATA_H, 2, adc_data);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI,
			"%s: read ADC data failed\n", __func__);
		goto _out;
	}

	battery_log(BAT_LOG_FULL,
		"%s: adc_sel = %d, adc_h = 0x%02X, adc_l = 0x%02X\n",
		__func__, adc_sel, adc_data[0], adc_data[1]);

	/* Calculate ADC value */
	*adc_val = (adc_data[0] * 256 + adc_data[1]) * rt9468_adc_unit[adc_sel]
		+ rt9468_adc_offset[adc_sel];

	ret = 0;

_out:
	info->i2c_log_level = BAT_LOG_FULL;
	mutex_unlock(&info->adc_access_lock);
	return ret;
}


static int rt9468_enable_watchdog_timer(struct rt9468_info *info, bool enable)
{
	int ret = 0;

	battery_log(BAT_LOG_CRTI, "%s: enable watchdog = %d\n",
		__func__, enable);
	ret = (enable ? rt9468_set_bit : rt9468_clr_bit)
		(info, RT9468_REG_CHG_CTRL13, RT9468_MASK_WDT_EN);

	return ret;
}

static int rt9468_is_charging_enable(struct rt9468_info *info, bool *enable)
{
	int ret = 0;

	ret = rt9468_i2c_read_byte(info, RT9468_REG_CHG_CTRL2);
	if (ret < 0)
		return ret;

	*enable = ((ret & RT9468_MASK_CHG_EN) >> RT9468_SHIFT_CHG_EN) > 0 ?
		true : false;

	return ret;
}

static int rt9468_enable_te(struct rt9468_info *info, bool enable)
{
	int ret = 0;

	ret = (enable ? rt9468_set_bit : rt9468_clr_bit)
		(info, RT9468_REG_CHG_CTRL2, RT9468_MASK_TE_EN);

	return ret;
}

static int rt9468_enable_pump_express(struct rt9468_info *info, bool enable)
{
	int ret = 0, i = 0, max_retry_times = 5;

	battery_log(BAT_LOG_CRTI, "%s: enable pumpX = %d\n", __func__, enable);

	ret = (enable ? rt9468_set_bit : rt9468_clr_bit)
		(info, RT9468_REG_CHG_CTRL16, RT9468_MASK_PUMPX_EN);
	if (ret < 0)
		return ret;

	for (i = 0; i < max_retry_times; i++) {
		msleep(2500);
		if (rt9468_i2c_test_bit(info, RT9468_REG_CHG_CTRL16,
			RT9468_SHIFT_PUMPX_UP) == 0)
			break;
	}
	if (i == max_retry_times) {
		battery_log(BAT_LOG_CRTI, "%s: pumpX wait failed\n", __func__);
		ret = -EIO;
	}

	return ret;
}

static int rt9468_get_ieoc(struct rt9468_info *info, u32 *ieoc)
{
	int ret = 0;
	u8 reg_ieoc = 0;

	ret = rt9468_i2c_read_byte(info, RT9468_REG_CHG_CTRL9);
	if (ret < 0)
		return ret;

	reg_ieoc = (ret & RT9468_MASK_IEOC) >> RT9468_SHIFT_IEOC;
	*ieoc = rt9468_find_closest_real_value(RT9468_IEOC_MIN, RT9468_IEOC_MAX,
		RT9468_IEOC_STEP, reg_ieoc);

	return ret;
}

static int rt9468_get_mivr(struct rt9468_info *info, u32 *mivr)
{
	int ret = 0;
	u8 reg_mivr = 0;

	ret = rt9468_i2c_read_byte(info, RT9468_REG_CHG_CTRL6);
	if (ret < 0)
		return ret;
	reg_mivr = ((ret & RT9468_MASK_MIVR) >> RT9468_SHIFT_MIVR) & 0xFF;

	*mivr = rt9468_find_closest_real_value(RT9468_MIVR_MIN, RT9468_MIVR_MAX,
		RT9468_MIVR_STEP, reg_mivr);

	return ret;
}

static int rt9468_set_ieoc(struct rt9468_info *info, const u32 ieoc)
{
	int ret = 0;

	/* Find corresponding reg value */
	u8 reg_ieoc = rt9468_find_closest_reg_value(RT9468_IEOC_MIN,
		RT9468_IEOC_MAX, RT9468_IEOC_STEP, RT9468_IEOC_NUM, ieoc);

	battery_log(BAT_LOG_CRTI, "%s: ieoc = %d\n", __func__, ieoc);

	ret = rt9468_i2c_update_bits(
		info,
		RT9468_REG_CHG_CTRL9,
		reg_ieoc << RT9468_SHIFT_IEOC,
		RT9468_MASK_IEOC
	);

	return ret;
}

static int rt9468_set_mivr(struct rt9468_info *info, const u32 mivr)
{
	int ret = 0, reg_mivr = 0;

	/* Find corresponding reg value */
	reg_mivr = rt9468_find_closest_reg_value(RT9468_MIVR_MIN,
		RT9468_MIVR_MAX, RT9468_MIVR_STEP, RT9468_MIVR_NUM, mivr);

	battery_log(BAT_LOG_CRTI, "%s: mivr = %d\n", __func__, mivr);

	ret = rt9468_i2c_update_bits(
		info,
		RT9468_REG_CHG_CTRL6,
		reg_mivr << RT9468_SHIFT_MIVR,
		RT9468_MASK_MIVR
	);

	return ret;
}

static int rt9468_get_charging_status(struct rt9468_info *info,
	enum rt9468_charging_status *chg_stat)
{
	int ret = 0;

	ret = rt9468_i2c_read_byte(info, RT9468_REG_CHG_STAT);
	if (ret < 0)
		return ret;

	*chg_stat = (ret & RT9468_MASK_CHG_STAT) >> RT9468_SHIFT_CHG_STAT;

	return ret;
}

static int rt9468_enable_usb_chrdet(struct rt9468_info *info, bool enable)
{
	int ret = 0;

	battery_log(BAT_LOG_CRTI, "%s: enable = %d\n", __func__, enable);
	ret = (enable ? rt9468_set_bit : rt9468_clr_bit)
		(info, RT9468_REG_CHG_DPDM1, RT9468_MASK_USBCHGEN);

	return ret;
}

static int rt9468_sw_init(struct rt9468_info *info)
{
	int ret = 0;
	struct rt9468_desc *desc = info->desc;
	bool enable_safety_timer = true;

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	/* Disable USB charger type detection */
	ret = rt9468_enable_usb_chrdet(info, false);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: disable usb chrdet failed\n",
			__func__);

	ret = rt9468_irq_init(info);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: irq init failed\n", __func__);

	/* Disable hardware ILIM */
	ret = rt9468_enable_ilim(info, false);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI,
			"%s: Disable ilim failed\n", __func__);

	/* Select IINLMTSEL to use AICR */
	ret = rt9468_select_input_current_limit(info, RT9468_IINLMTSEL_AICR);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: select iinlmtsel failed\n",
			__func__);

	ret = rt_charger_set_ichg(&info->mchr_info, &desc->ichg);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: Set ichg failed\n", __func__);

	ret = rt_charger_set_aicr(&info->mchr_info, &desc->aicr);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: Set aicr failed\n", __func__);

	ret = rt_charger_set_mivr(&info->mchr_info, &desc->mivr);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: Set mivr failed\n", __func__);

	ret = rt_charger_set_battery_voreg(&info->mchr_info, &desc->cv);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: set voreg failed\n", __func__);

	ret = rt9468_set_ieoc(info, desc->ieoc); /* mA */
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: Set ieoc failed\n", __func__);

	/* Enable TE */
	ret = rt9468_enable_te(info, desc->enable_te);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: Set te failed\n", __func__);

	/* Set fast charge timer to 12 hours */
	ret = rt9468_set_fast_charge_timer(info, desc->safety_timer);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI,
			"%s: Set fast timer failed\n", __func__);

	/* Enable charger timer */
	ret = rt_charger_enable_safety_timer(&info->mchr_info,
		&enable_safety_timer);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: enable charger timer failed\n",
			__func__);

	/* Enable watchdog timer */
	ret = rt9468_enable_watchdog_timer(info, desc->enable_wdt);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: enable watchdog timer failed\n",
			__func__);

	return ret;

}

/* Set register's value to default */
static int rt9468_hw_init(struct rt9468_info *info)
{
	int ret = 0;

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	ret = rt9468_set_bit(info, RT9468_REG_CORE_CTRL0, RT9468_MASK_RST);

	return ret;
}

static int rt9468_set_iin_vth(struct rt9468_info *info, u32 iin_vth)
{
	int ret = 0;
	u8 reg_iin_vth = 0;

	battery_log(BAT_LOG_CRTI, "%s: set iin_vth = %d\n", __func__, iin_vth);

	reg_iin_vth = rt9468_find_closest_reg_value(RT9468_IIN_VTH_MIN,
		RT9468_IIN_VTH_MAX, RT9468_IIN_VTH_STEP, RT9468_IIN_VTH_NUM,
		iin_vth);

	ret = rt9468_i2c_update_bits(
		info,
		RT9468_REG_CHG_CTRL14,
		reg_iin_vth << RT9468_SHIFT_IIN_VTH,
		RT9468_MASK_IIN_VTH
	);

	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: set iin vth failed, ret = %d\n",
			__func__, ret);

	return ret;
}

static void rt9468_otg_work_handler(struct work_struct *work)
{
	int ret = 0, i = 0;
	const unsigned int max_retry_cnt = 3;
	const int ss_wait_times = 5; /* soft start wait times */
	int vbus = 0;
	struct rt9468_info *info = (struct rt9468_info *)container_of(work,
		struct rt9468_info, otg_ctrl_work);

	ret = rt9468_enable_hidden_mode(info, true);
	if (ret < 0)
		return;

	battery_log(BAT_LOG_CRTI, "%s: start OTG work\n", __func__);

	if (info->to_enable_otg) {
		/* Wait for soft start finish interrupt */
		for (i = 0; i < ss_wait_times; i++) {
			ret = rt9468_get_adc(info, RT9468_ADC_VBUS_DIV2, &vbus);
			battery_log(BAT_LOG_CRTI, "%s: vbus = %dmV\n", __func__, vbus);
			if (ret == 0 && vbus > 4000) {
				/* Woraround reg[0x25] = 0x00 after entering OTG mode */
				ret = rt9468_i2c_write_byte(info, 0x25, 0x00);
				if (ret < 0)
					battery_log(BAT_LOG_CRTI,
						"%s: otg workaroud failed\n",
						__func__);
				else
					battery_log(BAT_LOG_CRTI,
						"%s: otg ss successfully\n",
						__func__);
				break;
			}
		}

		if (i == ss_wait_times) {
			battery_log(BAT_LOG_CRTI, "%s: otg ss failed\n",
				__func__);
			/* Disable OTG */
			rt9468_clr_bit(info, RT9468_REG_CHG_CTRL1,
				RT9468_MASK_OPA_MODE);
			ret = -EIO;
		}
		goto _out;
	}

	/* Disable OTG flow */

	/* Workaround: reg[0x25] = 0x0F after leaving OTG mode */
	ret = rt9468_i2c_write_byte(info, 0x25, 0x0F);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: otg workaroud failed\n",
			__func__);
		goto _out;
	}

	/* Set bit2 of reg[0x21] to 1 to enable discharging */
	ret = rt9468_set_bit(info, 0x21, 0x04);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: enable discharging failed\n",
			__func__);
		goto _out;
	}

	/* Wait for discharging to 0V */
	mdelay(20);

	for (i = 0; i < max_retry_cnt; i++) {
		/* Disable discharging */
		ret = rt9468_clr_bit(info, 0x21, 0x04);
		if (ret < 0)
			continue;
		if (rt9468_i2c_test_bit(info, 0x21, 2) == 0)
			break;
	}
	if (i == max_retry_cnt)
		battery_log(BAT_LOG_CRTI, "%s: disable discharging failed\n",
			__func__);

_out:
	rt9468_enable_hidden_mode(info, false);
	battery_log(BAT_LOG_CRTI, "%s: end OTG work\n", __func__);
}

static int rt9468_run_otg_work(struct rt9468_info *info)
{
	if (!queue_work(info->otg_ctrl_workqueue, &info->otg_ctrl_work))
		return -EINVAL;

	return 0;
}

static int rt9468_parse_dt(struct rt9468_info *info, struct device *dev)
{
	struct rt9468_desc *desc = NULL;
	struct device_node *np = dev->of_node;

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	if (!np) {
		battery_log(BAT_LOG_CRTI, "%s: no device node\n", __func__);
		return -EINVAL;
	}

	info->desc = &rt9468_default_desc;

	desc = devm_kzalloc(dev, sizeof(struct rt9468_desc), GFP_KERNEL);
	if (!desc) {
		battery_log(BAT_LOG_CRTI, "%s: no enough memory\n", __func__);
		return -ENOMEM;
	}

	if (of_property_read_string(np, "charger_name",
		&(info->mchr_info.name)) < 0) {
		battery_log(BAT_LOG_CRTI, "%s: no charger name\n", __func__);
		info->mchr_info.name = "rt9468";
	}

	if (of_property_read_u32(np, "regmap_represent_slave_addr",
		&desc->regmap_represent_slave_addr) < 0) {
		battery_log(BAT_LOG_CRTI, "%s: no regmap slave addr\n",
			__func__);
		desc->regmap_represent_slave_addr =
			rt9468_default_desc.regmap_represent_slave_addr;
	}

	if (of_property_read_string(np, "regmap_name",
		&(desc->regmap_name)) < 0) {
		battery_log(BAT_LOG_CRTI, "%s: no regmap name\n", __func__);
		desc->regmap_name = rt9468_default_desc.regmap_name;
	}

	if (of_property_read_u32(np, "ichg", &desc->ichg) < 0) {
		battery_log(BAT_LOG_CRTI, "%s: no ichg\n", __func__);
		desc->ichg = rt9468_default_desc.ichg;
	}

	if (of_property_read_u32(np, "aicr", &desc->aicr) < 0) {
		battery_log(BAT_LOG_CRTI, "%s: no aicr\n", __func__);
		desc->aicr = rt9468_default_desc.aicr;
	}

	if (of_property_read_u32(np, "mivr", &desc->mivr) < 0) {
		battery_log(BAT_LOG_CRTI, "%s: no mivr\n", __func__);
		desc->mivr = rt9468_default_desc.mivr;
	}

	if (of_property_read_u32(np, "cv", &desc->cv) < 0) {
		battery_log(BAT_LOG_CRTI, "%s: no cv\n", __func__);
		desc->cv = rt9468_default_desc.cv;
	}

	if (of_property_read_u32(np, "ieoc", &desc->ieoc) < 0) {
		battery_log(BAT_LOG_CRTI, "%s: no ieoc\n", __func__);
		desc->ieoc = rt9468_default_desc.ieoc;
	}

	if (of_property_read_u32(np, "safety_timer", &desc->safety_timer) < 0) {
		battery_log(BAT_LOG_CRTI, "%s: no safety timer\n", __func__);
		desc->safety_timer = rt9468_default_desc.safety_timer;
	}


	desc->enable_te = of_property_read_bool(np, "enable_te");
	desc->enable_wdt = of_property_read_bool(np, "enable_wdt");

	info->desc = desc;

	return 0;
}

static int rt9468_sw_reset(struct rt9468_info *info)
{
	int ret = 0;

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	/* Mask all irq */
	ret = rt9468_enable_all_irq(info, false);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: disable all irq failed\n",
			__func__);

	return ret;
}

/* ============================================================ */
/* The following is implementation for interface of mtk charger */
/* ============================================================ */

static int rt_charger_hw_init(struct mtk_charger_info *mchr_info, void *data)
{
	return 0;
}

static int rt_charger_dump_register(struct mtk_charger_info *mchr_info,
	void *data)
{
	int i = 0, ret = 0;
	u32 ichg = 0, aicr = 0, mivr = 0, ieoc = 0;
	bool chg_enable = 0;
	int adc_vsys = 0, adc_vbat = 0, adc_ibat = 0;
	enum rt9468_charging_status chg_status = RT9468_CHG_STATUS_READY;
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);
	ret = rt_charger_get_ichg(mchr_info, &ichg); /* 10uA */
	ret = rt_charger_get_aicr(mchr_info, &aicr); /* 10uA */
	ret = rt9468_get_charging_status(info, &chg_status);
	ret = rt9468_get_ieoc(info, &ieoc);
	ret = rt9468_get_mivr(info, &mivr);
	ret = rt9468_is_charging_enable(info, &chg_enable);
	ret = rt9468_get_adc(info, RT9468_ADC_VSYS, &adc_vsys);
	ret = rt9468_get_adc(info, RT9468_ADC_VBAT, &adc_vbat);
	ret = rt9468_get_adc(info, RT9468_ADC_IBAT, &adc_ibat);

	if ((Enable_BATDRV_LOG > BAT_LOG_CRTI) ||
	    (chg_status == RT9468_CHG_STATUS_FAULT)) {
		info->i2c_log_level = BAT_LOG_CRTI;
		for (i = 0; i < ARRAY_SIZE(rt9468_reg_addr); i++) {
			ret = rt9468_i2c_read_byte(info, rt9468_reg_addr[i]);
			if (ret < 0)
				return ret;
		}
	} else
		info->i2c_log_level = BAT_LOG_FULL;

	battery_log(BAT_LOG_CRTI,
		"%s: ICHG = %dmA, AICR = %dmA, MIVR = %dmV, IEOC = %dmA\n",
		__func__, ichg / 100, aicr / 100, mivr, ieoc);

	battery_log(BAT_LOG_CRTI,
		"%s: VSYS = %dmV, VBAT = %dmV IBAT = %dmA\n",
		__func__, adc_vsys, adc_vbat, adc_ibat);

	battery_log(BAT_LOG_CRTI,
		"%s: CHG_EN = %d, CHG_STATUS = %s\n",
		__func__, chg_enable, rt9468_chg_status_name[chg_status]);

	return ret;
}

static int rt_charger_enable_charging(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u8 enable = *((u8 *)data);
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	ret = (enable ? rt9468_set_bit : rt9468_clr_bit)
		(info, RT9468_REG_CHG_CTRL2, RT9468_MASK_CHG_EN);

	return ret;
}

static int rt_charger_enable_hz(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u8 enable = *((u8 *)data);
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	ret = (enable ? rt9468_set_bit : rt9468_clr_bit)
		(info, RT9468_REG_CHG_CTRL1, RT9468_MASK_HZ_EN);

	return ret;
}

static int rt_charger_enable_safety_timer(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u8 enable = *((u8 *)data);
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	ret = (enable ? rt9468_set_bit : rt9468_clr_bit)
		(info, RT9468_REG_CHG_CTRL12, RT9468_MASK_TMR_EN);

	return ret;
}

static int rt_charger_enable_otg(struct mtk_charger_info *mchr_info, void *data)
{
	u8 enable = *((u8 *)data);
	int ret = 0;
	u32 current_limit = 500; /* mA */
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	/* Set OTG_OC to 500mA */
	ret = rt_charger_set_boost_current_limit(mchr_info, &current_limit);
	if (ret < 0)
		return ret;

	/* Switch OPA mode to boost mode */
	battery_log(BAT_LOG_CRTI, "%s: otg enable = %d\n", __func__, enable);

	ret = (enable ? rt9468_set_bit : rt9468_clr_bit)
		(info, RT9468_REG_CHG_CTRL1, RT9468_MASK_OPA_MODE);

	info->to_enable_otg = enable;
	ret = rt9468_run_otg_work(info);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: run otg work failed\n",
			__func__);



	return ret;
}

static int rt_charger_enable_power_path(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u8 enable = *((u8 *)data);
	u32 mivr = (enable ? 4500 : RT9468_MIVR_MAX);
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	/* Use MIVR to disable power path for PD compliance */

	ret = rt9468_set_mivr(info, mivr);

	return ret;
}

static int rt_charger_enable_direct_charge(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u8 enable = *((u8 *)data);
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	battery_log(BAT_LOG_CRTI, "%s: enable direct charger = %d\n",
		__func__, enable);

	/* Clock select 3MHz */
	ret = rt9468_set_bit(info, RT9468_REG_CHG_PUMP, RT9468_MASK_VG_CLKSEL);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: clock select 3MHz failed\n", __func__);
		return ret;
	}

	if (enable) {
		/* Enable bypass mode */
		ret = rt9468_set_bit(info, RT9468_REG_CHG_CTRL2,
			RT9468_MASK_BYPASS_MODE);
		if (ret < 0) {
			battery_log(BAT_LOG_CRTI, "%s: enable bypass mode failed\n", __func__);
			return ret;
		}

		/* VG_ENSCP = 0 */
		ret = rt9468_clr_bit(info, RT9468_REG_CHG_PUMP, RT9468_MASK_VG_ENSCP);
		if (ret < 0) {
			battery_log(BAT_LOG_CRTI, "%s: disable VG_ENSCP failed\n", __func__);
			return ret;
		}

		/* VG_ENDISCHG = 0 */
		ret = rt9468_clr_bit(info, RT9468_REG_CHG_PUMP,
			RT9468_MASK_VG_ENDISCHG);
		if (ret < 0) {
			battery_log(BAT_LOG_CRTI, "%s: disable VG_ENDISCHG failed\n", __func__);
			return ret;
		}

		/* VG_EN = 1 */
		ret = rt9468_set_bit(info, RT9468_REG_CHG_PUMP,
			RT9468_MASK_VG_EN);
		if (ret < 0)
			battery_log(BAT_LOG_CRTI, "%s: enable VG_EN failed\n", __func__);

		return ret;
	}

	/* Disable direct charge */
	/* VG_EN = 0 */
	ret = rt9468_clr_bit(info, RT9468_REG_CHG_PUMP, RT9468_MASK_VG_EN);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: disable VG_EN failed\n", __func__);

	/* VG_ENDISCHG = 1 */
	ret = rt9468_set_bit(info, RT9468_REG_CHG_PUMP, RT9468_MASK_VG_ENDISCHG);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: enable VG_ENDISCHG failed\n", __func__);

	/* VG_ENSCP = 1 */
	ret = rt9468_set_bit(info, RT9468_REG_CHG_PUMP, RT9468_MASK_VG_ENSCP);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: enable VG_ENSCP failed\n", __func__);

	/* Disable bypass mode */
	ret = rt9468_clr_bit(info, RT9468_REG_CHG_CTRL2, RT9468_MASK_BYPASS_MODE);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: disable bypass mode failed\n", __func__);

	return ret;
}

static int rt_charger_set_ichg(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u32 ichg = 0;
	u8 reg_ichg = 0;
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	/* MTK's current unit : 10uA */
	/* Our current unit : mA */
	ichg = *((u32 *)data);
	ichg /= 100;

	/* Find corresponding reg value */
	reg_ichg = rt9468_find_closest_reg_value(RT9468_ICHG_MIN,
		RT9468_ICHG_MAX, RT9468_ICHG_STEP, RT9468_ICHG_NUM, ichg);

	battery_log(BAT_LOG_CRTI, "%s: ichg = %d\n", __func__, ichg);

	ret = rt9468_i2c_update_bits(
		info,
		RT9468_REG_CHG_CTRL7,
		reg_ichg << RT9468_SHIFT_ICHG,
		RT9468_MASK_ICHG
	);

	return ret;
}

static int rt_charger_set_aicr(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u32 aicr = 0;
	u8 reg_aicr = 0;
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	/* MTK's current unit : 10uA */
	/* Our current unit : mA */
	aicr = *((u32 *)data);
	aicr /= 100;

	/* Find corresponding reg value */
	reg_aicr = rt9468_find_closest_reg_value(RT9468_AICR_MIN,
		RT9468_AICR_MAX, RT9468_AICR_STEP, RT9468_AICR_NUM, aicr);

	battery_log(BAT_LOG_CRTI, "%s: aicr = %d\n", __func__, aicr);

	ret = rt9468_i2c_update_bits(
		info,
		RT9468_REG_CHG_CTRL3,
		reg_aicr << RT9468_SHIFT_AICR,
		RT9468_MASK_AICR
	);

	return ret;
}


static int rt_charger_set_mivr(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u32 mivr = *((u32 *)data);
	bool enable = true;
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	ret = rt_charger_is_power_path_enable(mchr_info, &enable);
	if (!enable) {
		battery_log(BAT_LOG_CRTI,
			"%s: power path is disabled, cannot adjust mivr\n",
			__func__);
		return -EINVAL;
	}

	ret = rt9468_set_mivr(info, mivr);


	return ret;
}

static int rt_charger_set_battery_voreg(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u32 voreg = 0;
	u8 reg_voreg = 0;
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	/* MTK's voltage unit : uV */
	/* Our voltage unit : mV */
	voreg = *((u32 *)data);
	voreg /= 1000;

	reg_voreg = rt9468_find_closest_reg_value(RT9468_BAT_VOREG_MIN,
		RT9468_BAT_VOREG_MAX, RT9468_BAT_VOREG_STEP,
		RT9468_BAT_VOREG_NUM, voreg);

	battery_log(BAT_LOG_CRTI, "%s: bat voreg = %d\n", __func__, voreg);

	ret = rt9468_i2c_update_bits(
		info,
		RT9468_REG_CHG_CTRL4,
		reg_voreg << RT9468_SHIFT_BAT_VOREG,
		RT9468_MASK_BAT_VOREG
	);

	return ret;
}


static int rt_charger_set_boost_current_limit(
	struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u8 reg_ilimit = 0;
	u32 current_limit = *((u32 *)data);
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	reg_ilimit = rt9468_find_closest_reg_value_via_table(
		rt9468_boost_oc_threshold,
		ARRAY_SIZE(rt9468_boost_oc_threshold),
		current_limit
	);

	battery_log(BAT_LOG_CRTI, "%s: boost ilimit = %d\n",
		__func__, current_limit);

	ret = rt9468_i2c_update_bits(
		info,
		RT9468_REG_CHG_CTRL10,
		reg_ilimit << RT9468_SHIFT_BOOST_OC,
		RT9468_MASK_BOOST_OC
	);

	return ret;
}

static int rt_charger_set_pep_current_pattern(
	struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u8 is_pump_up = *((u8 *)data);
	u32 ichg = 200000; /* 10uA */
	u32 aicr = 80000; /* 10uA */
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	battery_log(BAT_LOG_CRTI, "%s: pe1.0 pump_up = %d\n",
		__func__, is_pump_up);

	/* Set Pump Up/Down */
	ret = (is_pump_up ?
		rt9468_set_bit(info, RT9468_REG_CHG_CTRL16, RT9468_MASK_PUMPX_UP)
		: rt9468_set_bit(info, RT9468_REG_CHG_CTRL16, RT9468_MASK_PUMPX_DN));
	if (ret < 0)
		return ret;

	ret = rt_charger_set_aicr(mchr_info, &aicr);
	if (ret < 0)
		return ret;

	ret = rt_charger_set_ichg(mchr_info, &ichg);
	if (ret < 0)
		return ret;

	/* Enable PumpX */
	ret = rt9468_enable_pump_express(info, true);

	return ret;
}

static int rt_charger_set_pep20_reset(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u32 mivr = 4500; /* mA */
	u32 ichg = 51200; /* 10uA */
	u32 aicr = 10000; /* 10uA */

	ret = rt_charger_set_mivr(mchr_info, &mivr);
	if (ret < 0)
		return ret;

	ret = rt_charger_set_ichg(mchr_info, &ichg);
	if (ret < 0)
		return ret;

	ret = rt_charger_set_aicr(mchr_info, &aicr);
	if (ret < 0)
		return ret;

	msleep(200);

	aicr = 70000;
	ret = rt_charger_set_aicr(mchr_info, &aicr);

	return ret;
}

static struct timespec ptime[13];
static int cptime[13][2];

static int dtime(int i)
{
	struct timespec time;

	time = timespec_sub(ptime[i], ptime[i-1]);
	return time.tv_nsec/1000000;
}

#define PEOFFTIME 40
#define PEONTIME 90

static int rt_charger_set_pep20_current_pattern(
	struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	int value;
	int i, j = 0;
	int flag;
	u32 voltage = *((u32 *)data);
	u32 ichg = 0; /* 10uA */
	u32 aicr = 0; /* 10uA */
	u32 mivr = 0; /* mV */

	battery_log(BAT_LOG_CRTI, "%s: pep2.0  = %d\n", __func__, voltage);

	mivr = 4500;
	rt_charger_set_mivr(mchr_info, &mivr);

	ichg = 51200;
	rt_charger_set_ichg(mchr_info, &ichg);

	usleep_range(1000, 1200);
	value = (voltage - CHR_VOLT_05_500000_V) / CHR_VOLT_00_500000_V;

	aicr = 10000;
	rt_charger_set_aicr(mchr_info, &aicr);
	msleep(70);

	get_monotonic_boottime(&ptime[j++]);
	for (i = 4; i >= 0; i--) {
		flag = value & (1 << i);

		if (flag == 0) {
			aicr = 70000;
			rt_charger_set_aicr(mchr_info, &aicr);
			msleep(PEOFFTIME);
			get_monotonic_boottime(&ptime[j]);
			cptime[j][0] = PEOFFTIME;
			cptime[j][1] = dtime(j);
			if (cptime[j][1] < 30 || cptime[j][1] > 65) {
				battery_log(BAT_LOG_CRTI,
					"%s: fail1, idx:%d target:%d actual:%d\n",
					__func__, i, PEOFFTIME, cptime[j][1]);
				return -EIO;
			}
			j++;
			aicr = 10000;
			rt_charger_set_aicr(mchr_info, &aicr);
			msleep(PEONTIME);
			get_monotonic_boottime(&ptime[j]);
			cptime[j][0] = PEONTIME;
			cptime[j][1] = dtime(j);
			if (cptime[j][1] < 90 || cptime[j][1] > 115) {
				battery_log(BAT_LOG_CRTI,
					"%s: fail2, idx:%d target:%d actual:%d\n",
					__func__, i, PEOFFTIME, cptime[j][1]);
				return -EIO;
			}
			j++;

		} else {
			aicr = 70000;
			rt_charger_set_aicr(mchr_info, &aicr);
			msleep(PEONTIME);
			get_monotonic_boottime(&ptime[j]);
			cptime[j][0] = PEONTIME;
			cptime[j][1] = dtime(j);
			if (cptime[j][1] < 90 || cptime[j][1] > 115) {
				battery_log(BAT_LOG_CRTI,
					"%s: fail3, idx:%d target:%d actual:%d\n",
					__func__, i, PEOFFTIME, cptime[j][1]);
				return -EIO;
			}
			j++;
			aicr = 10000;
			rt_charger_set_aicr(mchr_info, &aicr);
			msleep(PEOFFTIME);
			get_monotonic_boottime(&ptime[j]);
			cptime[j][0] = PEOFFTIME;
			cptime[j][1] = dtime(j);
			if (cptime[j][1] < 30 || cptime[j][1] > 65) {
				battery_log(BAT_LOG_CRTI,
					"%s: fail4, idx:%d target:%d actual:%d\n",
					__func__, i, PEOFFTIME, cptime[j][1]);
				return -EIO;
			}
			j++;
		}
	}

	aicr = 70000;
	rt_charger_set_aicr(mchr_info, &aicr);
	msleep(160);
	get_monotonic_boottime(&ptime[j]);
	cptime[j][0] = 160;
	cptime[j][1] = dtime(j);
	if (cptime[j][1] < 150 || cptime[j][1] > 240) {
		battery_log(BAT_LOG_CRTI,
			"%s: fail5, idx:%d target:%d actual:%d\n",
			__func__, i, PEOFFTIME, cptime[j][1]);
		return -EIO;
	}
	j++;

	aicr = 10000;
	rt_charger_set_aicr(mchr_info, &aicr);
	msleep(30);
	aicr = 70000;
	rt_charger_set_aicr(mchr_info, &aicr);

	battery_log(BAT_LOG_CRTI,
		"%s: voltage:%d bit:%d time:%3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d!!\n",
		__func__, voltage, value,
	cptime[1][0], cptime[2][0], cptime[3][0], cptime[4][0], cptime[5][0],
	cptime[6][0], cptime[7][0], cptime[8][0], cptime[9][0], cptime[10][0], cptime[11][0]);

	battery_log(BAT_LOG_CRTI,
		"%s: voltage:%d bit:%d time:%3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d!!\n",
		__func__, voltage, value,
	cptime[1][1], cptime[2][1], cptime[3][1], cptime[4][1], cptime[5][1],
	cptime[6][1], cptime[7][1], cptime[8][1], cptime[9][1], cptime[10][1], cptime[11][1]);

	aicr = 325000;
	rt_charger_set_aicr(mchr_info, &aicr);

	return ret;
}

static int rt_charger_get_ichg(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u8 reg_ichg = 0;
	u32 ichg = 0;
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	ret = rt9468_i2c_read_byte(info, RT9468_REG_CHG_CTRL7);
	if (ret < 0)
		return ret;

	reg_ichg = (ret & RT9468_MASK_ICHG) >> RT9468_SHIFT_ICHG;
	ichg = rt9468_find_closest_real_value(RT9468_ICHG_MIN, RT9468_ICHG_MAX,
		RT9468_ICHG_STEP, reg_ichg);

	/* MTK's current unit : 10uA */
	/* Our current unit : mA */
	ichg *= 100;
	*((u32 *)data) = ichg;

	return ret;
}


static int rt_charger_get_aicr(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u8 reg_aicr = 0;
	u32 aicr = 0;
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	ret = rt9468_i2c_read_byte(info, RT9468_REG_CHG_CTRL3);
	if (ret < 0)
		return ret;

	reg_aicr = (ret & RT9468_MASK_AICR) >> RT9468_SHIFT_AICR;
	aicr = rt9468_find_closest_real_value(RT9468_AICR_MIN, RT9468_AICR_MAX,
		RT9468_AICR_STEP, reg_aicr);

	/* MTK's current unit : 10uA */
	/* Our current unit : mA */
	aicr *= 100;
	*((u32 *)data) = aicr;

	return ret;
}

static int rt_charger_get_temperature(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0, adc_temp = 0;
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	/* Get value from ADC */
	ret = rt9468_get_adc(info, RT9468_ADC_TEMP_JC, &adc_temp);
	if (ret < 0)
		return ret;

	/* Workaround for RT9468 shuttle, always return 30 degree */
	adc_temp = 30;
	((int *)data)[0] = adc_temp;
	((int *)data)[1] = adc_temp;

	battery_log(BAT_LOG_CRTI, "%s: temperature = %d\n", __func__, adc_temp);

	return ret;
}

static int rt_charger_get_ibus(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0, adc_ibus = 0;
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	/* Get value from ADC */
	ret = rt9468_get_adc(info, RT9468_ADC_IBUS, &adc_ibus);
	if (ret < 0)
		return ret;

	*((u32 *)data) = adc_ibus;

	battery_log(BAT_LOG_CRTI, "%s: ibus = %dmA\n", __func__, adc_ibus);
	return ret;
}

static int rt_charger_get_vbus(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0, adc_vbus = 0;
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	/* Get value from ADC */
	ret = rt9468_get_adc(info, RT9468_ADC_VBUS_DIV2, &adc_vbus);
	if (ret < 0)
		return ret;

	*((u32 *)data) = adc_vbus;

	battery_log(BAT_LOG_FULL, "%s: vbus = %dmA\n", __func__, adc_vbus);
	return ret;
}

static int rt_charger_is_charging_done(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	enum rt9468_charging_status chg_stat = RT9468_CHG_STATUS_READY;
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	ret = rt9468_get_charging_status(info, &chg_stat);

	/* Return is charging done or not */
	switch (chg_stat) {
	case RT9468_CHG_STATUS_READY:
	case RT9468_CHG_STATUS_PROGRESS:
	case RT9468_CHG_STATUS_FAULT:
		*((u32 *)data) = 0;
		break;
	case RT9468_CHG_STATUS_DONE:
		*((u32 *)data) = 1;
		break;
	default:
		*((u32 *)data) = 0;
		break;
	}

	return ret;
}

static int rt_charger_is_power_path_enable(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u32 mivr = 0;
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	ret = rt9468_get_mivr(info, &mivr);
	if (ret < 0)
		return ret;

	*((bool *)data) = ((mivr == RT9468_MIVR_MAX) ? false : true);

	return ret;
}

static int rt_charger_is_safety_timer_enable(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	ret = rt9468_i2c_test_bit(info, RT9468_REG_CHG_CTRL12,
		RT9468_SHIFT_TMR_EN);
	if (ret < 0)
		return ret;

	*((bool *)data) = (ret > 0) ? true : false;

	return ret;
}

static int rt_charger_reset_watchdog_timer(struct mtk_charger_info *mchr_info,
	void *data)
{
	/* Any I2C communication can reset watchdog timer */
	int ret = 0;
	enum rt9468_charging_status chg_status;
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	ret = rt9468_get_charging_status(info, &chg_status);

	return ret;
}

static int rt_charger_run_aicl(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0, i = 0;
	u32 mivr = 0, iin_vth = 0, aicr = 0;
	const u32 max_wait_time = 5;
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	/* Check whether MIVR loop is active */
	if (rt9468_i2c_test_bit(info, RT9468_REG_CHG_STATC,
		RT9468_SHIFT_CHG_MIVR) <= 0) {
		battery_log(BAT_LOG_CRTI, "%s: mivr loop is not active\n",
			__func__);
		return 0;
	}

	battery_log(BAT_LOG_CRTI, "%s: mivr loop is active\n", __func__);
	ret = rt9468_get_mivr(info, &mivr);
	if (ret < 0)
		goto _out;

	/* Check if there's a suitable IIN_VTH */
	iin_vth = mivr + 200;
	if (iin_vth > RT9468_IIN_VTH_MAX) {
		battery_log(BAT_LOG_CRTI, "%s: no suitable IIN_VTH, vth = %d\n",
			__func__, iin_vth);
		ret = -EINVAL;
		goto _out;
	}

	ret = rt9468_set_iin_vth(info, iin_vth);
	if (ret < 0)
		goto _out;

	ret = rt9468_set_bit(info, RT9468_REG_CHG_CTRL14, RT9468_MASK_IIN_MEAS);
	if (ret < 0)
		goto _out;

	for (i = 0; i < max_wait_time; i++) {
		msleep(500);
		if (rt9468_i2c_test_bit(info, RT9468_REG_CHG_CTRL14,
			RT9468_SHIFT_IIN_MEAS) == 0)
			break;
	}
	if (i == max_wait_time) {
		battery_log(BAT_LOG_CRTI, "%s: wait AICL time out\n",
			__func__);
		ret = -EIO;
		goto _out;
	}

	ret = rt_charger_get_aicr(mchr_info, &aicr);
	if (ret < 0)
		goto _out;

	*((u32 *)data) = aicr / 100;
	battery_log(BAT_LOG_CRTI, "%s: OK, aicr upper bound = %dmA\n",
		__func__, aicr / 100);

	return 0;

_out:
	*((u32 *)data) = 0;
	return ret;
}

static int rt_charger_set_pep20_efficiency_table(
	struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	pep20_profile_t *profile = (pep20_profile_t *)data;

	battery_log(BAT_LOG_CRTI, "%s: start\n", __func__);
	profile[0].vchr = 8000;
	profile[1].vchr = 8000;
	profile[2].vchr = 8000;
	profile[3].vchr = 8500;
	profile[4].vchr = 8500;
	profile[5].vchr = 8500;
	profile[6].vchr = 9000;
	profile[7].vchr = 9000;
	profile[8].vchr = 9500;
	profile[9].vchr = 9500;

	return ret;
}

static int rt_charger_set_error_state(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	struct rt9468_info *info = (struct rt9468_info *)mchr_info;

	info->err_state = *((bool *)data);
	rt_charger_enable_hz(mchr_info, &info->err_state);

	return ret;
}

static const mtk_charger_intf rt9468_mchr_intf[CHARGING_CMD_NUMBER] = {
	[CHARGING_CMD_INIT] = rt_charger_hw_init,
	[CHARGING_CMD_DUMP_REGISTER] = rt_charger_dump_register,
	[CHARGING_CMD_ENABLE] = rt_charger_enable_charging,
	[CHARGING_CMD_SET_HIZ_SWCHR] = rt_charger_enable_hz,
	[CHARGING_CMD_ENABLE_SAFETY_TIMER] = rt_charger_enable_safety_timer,
	[CHARGING_CMD_ENABLE_OTG] = rt_charger_enable_otg,
	[CHARGING_CMD_ENABLE_POWER_PATH] = rt_charger_enable_power_path,
	[CHARGING_CMD_ENABLE_DIRECT_CHARGE] = rt_charger_enable_direct_charge,
	[CHARGING_CMD_SET_CURRENT] = rt_charger_set_ichg,
	[CHARGING_CMD_SET_INPUT_CURRENT] = rt_charger_set_aicr,
	[CHARGING_CMD_SET_VINDPM] = rt_charger_set_mivr,
	[CHARGING_CMD_SET_CV_VOLTAGE] = rt_charger_set_battery_voreg,
	[CHARGING_CMD_SET_BOOST_CURRENT_LIMIT] = rt_charger_set_boost_current_limit,
	[CHARGING_CMD_SET_TA_CURRENT_PATTERN] = rt_charger_set_pep_current_pattern,
	[CHARGING_CMD_SET_TA20_RESET] = rt_charger_set_pep20_reset,
	[CHARGING_CMD_SET_TA20_CURRENT_PATTERN] = rt_charger_set_pep20_current_pattern,
	[CHARGING_CMD_SET_ERROR_STATE] = rt_charger_set_error_state,
	[CHARGING_CMD_GET_CURRENT] = rt_charger_get_ichg,
	[CHARGING_CMD_GET_INPUT_CURRENT] = rt_charger_get_aicr,
	[CHARGING_CMD_GET_CHARGER_TEMPERATURE] = rt_charger_get_temperature,
	[CHARGING_CMD_GET_CHARGING_STATUS] = rt_charger_is_charging_done,
	[CHARGING_CMD_GET_IS_POWER_PATH_ENABLE] = rt_charger_is_power_path_enable,
	[CHARGING_CMD_GET_IS_SAFETY_TIMER_ENABLE] = rt_charger_is_safety_timer_enable,
	[CHARGING_CMD_RESET_WATCH_DOG_TIMER] = rt_charger_reset_watchdog_timer,
	[CHARGING_CMD_GET_IBUS] = rt_charger_get_ibus,
	[CHARGING_CMD_GET_VBUS] = rt_charger_get_vbus,
	[CHARGING_CMD_RUN_AICL] = rt_charger_run_aicl,
	[CHARGING_CMD_SET_PEP20_EFFICIENCY_TABLE] = rt_charger_set_pep20_efficiency_table,

	/*
	 * The following interfaces are not related to charger
	 * Define in .c
	 */
	[CHARGING_CMD_SW_INIT] = mtk_charger_sw_init,
	[CHARGING_CMD_SET_HV_THRESHOLD] = mtk_charger_set_hv_threshold,
	[CHARGING_CMD_GET_HV_STATUS] = mtk_charger_get_hv_status,
	[CHARGING_CMD_GET_BATTERY_STATUS] = mtk_charger_get_battery_status,
	[CHARGING_CMD_GET_CHARGER_DET_STATUS] = mtk_charger_get_charger_det_status,
	[CHARGING_CMD_GET_CHARGER_TYPE] = mtk_charger_get_charger_type,
	[CHARGING_CMD_GET_IS_PCM_TIMER_TRIGGER] = mtk_charger_get_is_pcm_timer_trigger,
	[CHARGING_CMD_SET_PLATFORM_RESET] = mtk_charger_set_platform_reset,
	[CHARGING_CMD_GET_PLATFORM_BOOT_MODE] = mtk_charger_get_platform_boot_mode,
	[CHARGING_CMD_SET_POWER_OFF] = mtk_charger_set_power_off,
	[CHARGING_CMD_GET_POWER_SOURCE] = mtk_charger_get_power_source,
	[CHARGING_CMD_GET_CSDAC_FALL_FLAG] = mtk_charger_get_csdac_full_flag,
	[CHARGING_CMD_DISO_INIT] = mtk_charger_diso_init,
	[CHARGING_CMD_GET_DISO_STATE] = mtk_charger_get_diso_state,
	[CHARGING_CMD_SET_VBUS_OVP_EN] = mtk_charger_set_vbus_ovp_en,
	[CHARGING_CMD_GET_BIF_VBAT] = mtk_charger_get_bif_vbat,
	[CHARGING_CMD_SET_CHRIND_CK_PDN] = mtk_charger_set_chrind_ck_pdn,
	[CHARGING_CMD_GET_BIF_TBAT] = mtk_charger_get_bif_tbat,
	[CHARGING_CMD_SET_DP] = mtk_charger_set_dp,
	[CHARGING_CMD_GET_BIF_IS_EXIST] = mtk_charger_get_bif_is_exist,
};

/* ========================= */
/* i2c driver function */
/* ========================= */


static int rt9468_probe(struct i2c_client *i2c,
	const struct i2c_device_id *dev_id)
{
	int ret = 0;
	struct rt9468_info *info = NULL;

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	info = devm_kzalloc(&i2c->dev, sizeof(struct rt9468_info), GFP_KERNEL);
	if (!info) {
		battery_log(BAT_LOG_CRTI, "%s: no enough memory", __func__);
		return -ENOMEM;
	}
	info->i2c = i2c;
	info->i2c_log_level = BAT_LOG_FULL;
	mutex_init(&info->i2c_access_lock);
	mutex_init(&info->adc_access_lock);

	/* Is HW exist */
	if (!rt9468_is_hw_exist(info)) {
		battery_log(BAT_LOG_CRTI, "%s: no rt9468 exists\n", __func__);
		ret = -ENODEV;
		goto err_no_dev;
	}
	i2c_set_clientdata(i2c, info);

	ret = rt9468_parse_dt(info, &i2c->dev);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: parse dt failed\n", __func__);
		goto err_parse_dt;
	}

	ret = rt9468_irq_register(info);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: irq init failed\n", __func__);
		goto err_irq_register;
	}

#ifdef CONFIG_RT_REGMAP
	ret = rt9468_register_rt_regmap(info);
	if (ret < 0)
		goto err_register_regmap;
#endif

	ret = rt9468_hw_init(info);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI,
			"%s: set register to default value failed\n", __func__);

	ret = rt9468_sw_init(info);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: set failed\n", __func__);
		goto err_sw_init;
	}

	ret = rt9468_sw_workaround(info);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: software workaround failed\n",
			__func__);
		goto err_sw_workaround;
	}

	/* Create single thread workqueue */
	info->otg_ctrl_workqueue =
		create_singlethread_workqueue("otg_ctrl_workqueue");
	if (!info->otg_ctrl_workqueue) {
		ret = -ENOMEM;
		goto err_create_wq;
	}

	/* Init discharging workqueue */
	INIT_WORK(&info->otg_ctrl_work,
		rt9468_otg_work_handler);

	rt_charger_dump_register(&info->mchr_info, NULL);

	/* Hook chr_control_interface with battery's interface */
	info->mchr_info.mchr_intf = rt9468_mchr_intf;
	mtk_charger_set_info(&info->mchr_info);
	battery_log(BAT_LOG_CRTI, "%s: ends\n", __func__);

	return ret;

err_sw_init:
err_sw_workaround:
err_create_wq:
#ifdef CONFIG_RT_REGMAP
	rt_regmap_device_unregister(info->regmap_dev);
err_register_regmap:
#endif
err_no_dev:
err_parse_dt:
err_irq_register:
	mutex_destroy(&info->i2c_access_lock);
	mutex_destroy(&info->adc_access_lock);
	return ret;
}


static int rt9468_remove(struct i2c_client *i2c)
{
	int ret = 0;
	struct rt9468_info *info = i2c_get_clientdata(i2c);

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

#ifdef CONFIG_RT_REGMAP
	rt_regmap_device_unregister(info->regmap_dev);
#endif
	mutex_destroy(&info->i2c_access_lock);
	mutex_destroy(&info->adc_access_lock);

	return ret;
}

static void rt9468_shutdown(struct i2c_client *i2c)
{
	int ret = 0;
	struct rt9468_info *info = i2c_get_clientdata(i2c);

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	if (info && info->irq) {
		/* Set log level to CRITICAL */
		info->i2c_log_level = BAT_LOG_CRTI;
		ret = rt9468_sw_reset(info);
		if (ret < 0)
			battery_log(BAT_LOG_CRTI, "%s: sw reset failed\n", __func__);

		disable_irq(info->irq);
	}
}

static const struct i2c_device_id rt9468_i2c_id[] = {
	{"rt9468", 0},
	{}
};

#ifdef CONFIG_OF
static const struct of_device_id rt9468_of_match[] = {
	{ .compatible = "mediatek,swithing_charger", },
	{},
};
#else

#define RT9468_BUSNUM 1

static struct i2c_board_info rt9468_i2c_board_info __initdata = {
	I2C_BOARD_INFO("rt9468", RT9468_SALVE_ADDR)
};
#endif /* CONFIG_OF */


static struct i2c_driver rt9468_i2c_driver = {
	.driver = {
		.name = "rt9468_shuttle",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = rt9468_of_match,
#endif
	},
	.probe = rt9468_probe,
	.remove = rt9468_remove,
	.shutdown = rt9468_shutdown,
	.id_table = rt9468_i2c_id,
};


static int __init rt9468_init(void)
{
	int ret = 0;

#ifdef CONFIG_OF
	battery_log(BAT_LOG_CRTI, "%s: with dts\n", __func__);
#else
	battery_log(BAT_LOG_CRTI, "%s: without dts\n", __func__);
	i2c_register_board_info(RT9468_BUSNUM, &rt9468_i2c_board_info, 1);
#endif

	ret = i2c_add_driver(&rt9468_i2c_driver);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: register i2c driver failed\n",
			__func__);

	return ret;
}
module_init(rt9468_init);


static void __exit rt9468_exit(void)
{
	i2c_del_driver(&rt9468_i2c_driver);
}
module_exit(rt9468_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("ShuFanLee <shufan_lee@richtek.com>");
MODULE_DESCRIPTION("RT9468 Charger Driver Shuttle E1");
MODULE_VERSION("1.0.0_MTK");
