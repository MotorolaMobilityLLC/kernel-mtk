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
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <mt-plat/charging.h>
#include <mt-plat/battery_common.h>
#include <mt-plat/mt_boot_common.h>
#include <mach/mt_pe.h>
#include "rt9466.h"
#include "mtk_charger_intf.h"

#ifdef CONFIG_RT_REGMAP
#include <mt-plat/rt-regmap.h>
#endif

#define I2C_ACCESS_MAX_RETRY	5

#define RT9466_DRV_VERSION "1.0.6_MTK"

/* ======================= */
/* RT9466 Parameter        */
/* ======================= */

static const u32 rt9466_boost_oc_threshold[] = {
	500, 700, 1100, 1300, 1800, 2100, 2400, 3000,
}; /* mA */

/*
 * Not to unmask the following IRQs
 * PWR_RDYM/CHG_MIVRM/CHG_AICRM
 * VBUSOVM
 * TS_BAT_HOTM/TS_BAT_WARMM/TS_BAT_COOLM/TS_BAT_COLDM
 * CHG_OTPM/CHG_RVPM/CHG_ADPBADM/CHG_STATCM/CHG_FAULTM/TS_STATCM
 * IEOCM/TERMM/SSFINISHM/SSFINISHM/AICLMeasM
 * BST_BATUVM/PUMPX_DONEM/ADC_DONEM
 */
static const u8 rt9466_irq_init_data[] = {
	RT9466_MASK_PWR_RDYM | RT9466_MASK_CHG_MIVRM | RT9466_MASK_CHG_AICRM,
	RT9466_MASK_VBUSOVM,
	RT9466_MASK_TS_BAT_HOTM | RT9466_MASK_TS_BAT_WARMM |
	RT9466_MASK_TS_BAT_COOLM | RT9466_MASK_TS_BAT_COLDM |
	RT9466_MASK_TS_STATC_RESERVED,
	RT9466_MASK_CHG_OTPM | RT9466_MASK_CHG_RVPM | RT9466_MASK_CHG_ADPBADM |
	RT9466_MASK_CHG_STATCM | RT9466_MASK_CHG_FAULTM | RT9466_MASK_TS_STATCM,
	RT9466_MASK_IEOCM | RT9466_MASK_TERMM | RT9466_MASK_SSFINISHM |
	RT9466_MASK_CHG_AICLMEASM | RT9466_MASK_IRQ2_RESERVED,
	RT9466_MASK_BST_BATUVM | RT9466_MASK_PUMPX_DONEM |
	RT9466_MASK_ADC_DONEM | RT9466_MASK_IRQ3_RESERVED,
};

static const u8 rt9466_irq_maskall_data[] = {
	0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF
};

enum rt9466_charging_status {
	RT9466_CHG_STATUS_READY = 0,
	RT9466_CHG_STATUS_PROGRESS,
	RT9466_CHG_STATUS_DONE,
	RT9466_CHG_STATUS_FAULT,
	RT9466_CHG_STATUS_MAX,
};

/* Charging status name */
static const char *rt9466_chg_status_name[RT9466_CHG_STATUS_MAX] = {
	"ready", "progress", "done", "fault",
};

/* ======================= */
/* Address & Default value */
/* ======================= */

static const unsigned char rt9466_reg_addr[] = {
	RT9466_REG_CORE_CTRL0,
	RT9466_REG_CHG_CTRL1,
	RT9466_REG_CHG_CTRL2,
	RT9466_REG_CHG_CTRL3,
	RT9466_REG_CHG_CTRL4,
	RT9466_REG_CHG_CTRL5,
	RT9466_REG_CHG_CTRL6,
	RT9466_REG_CHG_CTRL7,
	RT9466_REG_CHG_CTRL8,
	RT9466_REG_CHG_CTRL9,
	RT9466_REG_CHG_CTRL10,
	RT9466_REG_CHG_CTRL11,
	RT9466_REG_CHG_CTRL12,
	RT9466_REG_CHG_CTRL13,
	RT9466_REG_CHG_CTRL14,
	RT9466_REG_CHG_CTRL15,
	RT9466_REG_CHG_CTRL16,
	RT9466_REG_CHG_ADC,
	RT9466_REG_CHG_CTRL19,
	RT9466_REG_CHG_CTRL17,
	RT9466_REG_CHG_CTRL18,
	RT9466_REG_DEVICE_ID,
	RT9466_REG_CHG_STAT,
	RT9466_REG_CHG_NTC,
	RT9466_REG_ADC_DATA_H,
	RT9466_REG_ADC_DATA_L,
	RT9466_REG_CHG_STATC,
	RT9466_REG_CHG_FAULT,
	RT9466_REG_TS_STATC,
	RT9466_REG_CHG_IRQ1,
	RT9466_REG_CHG_IRQ2,
	RT9466_REG_CHG_IRQ3,
	RT9466_REG_CHG_STATC_CTRL,
	RT9466_REG_CHG_FAULT_CTRL,
	RT9466_REG_TS_STATC_CTRL,
	RT9466_REG_CHG_IRQ1_CTRL,
	RT9466_REG_CHG_IRQ2_CTRL,
	RT9466_REG_CHG_IRQ3_CTRL,
};


static const u8 rt9466_reg_en_hidden_mode[] = {
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
};

static const u8 rt9466_val_en_hidden_mode[] = {
	0x49, 0x32, 0xB6, 0x27, 0x48, 0x18, 0x03, 0xE2,
};

enum rt9466_iin_limit_sel {
	RT9466_IIMLMTSEL_PSEL_OTG,
	RT9466_IINLMTSEL_AICR = 2,
	RT9466_IINLMTSEL_LOWER_LEVEL, /* lower of above two */
};

enum rt9466_adc_sel {
	RT9466_ADC_VBUS_DIV5 = 1,
	RT9466_ADC_VBUS_DIV2,
	RT9466_ADC_VSYS,
	RT9466_ADC_VBAT,
	RT9466_ADC_TS_BAT = 6,
	RT9466_ADC_IBUS = 8,
	RT9466_ADC_IBAT,
	RT9466_ADC_REGN = 11,
	RT9466_ADC_TEMP_JC,
	RT9466_ADC_MAX,
};

/* Unit for each ADC parameter
 * 0 stands for reserved
 * For TS_BAT, the real unit is 0.25.
 * Here we use 25, please remember to divide 100 while showing the value
 */
static const int rt9466_adc_unit[RT9466_ADC_MAX] = {
	0,
	RT9466_ADC_UNIT_VBUS_DIV5,
	RT9466_ADC_UNIT_VBUS_DIV2,
	RT9466_ADC_UNIT_VSYS,
	RT9466_ADC_UNIT_VBAT,
	0,
	RT9466_ADC_UNIT_TS_BAT,
	0,
	RT9466_ADC_UNIT_IBUS,
	RT9466_ADC_UNIT_IBAT,
	0,
	RT9466_ADC_UNIT_REGN,
	RT9466_ADC_UNIT_TEMP_JC,
};

static const int rt9466_adc_offset[RT9466_ADC_MAX] = {
	0,
	RT9466_ADC_OFFSET_VBUS_DIV5,
	RT9466_ADC_OFFSET_VBUS_DIV2,
	RT9466_ADC_OFFSET_VSYS,
	RT9466_ADC_OFFSET_VBAT,
	0,
	RT9466_ADC_OFFSET_TS_BAT,
	0,
	RT9466_ADC_OFFSET_IBUS,
	RT9466_ADC_OFFSET_IBAT,
	0,
	RT9466_ADC_OFFSET_REGN,
	RT9466_ADC_OFFSET_TEMP_JC,
};

/* ========= */
/* RT Regmap */
/* ========= */

#ifdef CONFIG_RT_REGMAP
RT_REG_DECL(RT9466_REG_CORE_CTRL0, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL3, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL4, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL5, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL6, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL7, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL8, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL9, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL10, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL11, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL12, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL13, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL14, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL15, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL16, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_ADC, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL19, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL17, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL18, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_DEVICE_ID, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_STAT, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_NTC, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_ADC_DATA_H, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_ADC_DATA_L, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_STATC, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_FAULT, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_TS_STATC, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_IRQ1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_IRQ2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_IRQ3, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_STATC_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_FAULT_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_TS_STATC_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_IRQ1_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_IRQ2_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_IRQ3_CTRL, 1, RT_VOLATILE, {});

static rt_register_map_t rt9466_regmap_map[] = {
	RT_REG(RT9466_REG_CORE_CTRL0),
	RT_REG(RT9466_REG_CHG_CTRL1),
	RT_REG(RT9466_REG_CHG_CTRL2),
	RT_REG(RT9466_REG_CHG_CTRL3),
	RT_REG(RT9466_REG_CHG_CTRL4),
	RT_REG(RT9466_REG_CHG_CTRL5),
	RT_REG(RT9466_REG_CHG_CTRL6),
	RT_REG(RT9466_REG_CHG_CTRL7),
	RT_REG(RT9466_REG_CHG_CTRL8),
	RT_REG(RT9466_REG_CHG_CTRL9),
	RT_REG(RT9466_REG_CHG_CTRL10),
	RT_REG(RT9466_REG_CHG_CTRL11),
	RT_REG(RT9466_REG_CHG_CTRL12),
	RT_REG(RT9466_REG_CHG_CTRL13),
	RT_REG(RT9466_REG_CHG_CTRL14),
	RT_REG(RT9466_REG_CHG_CTRL15),
	RT_REG(RT9466_REG_CHG_CTRL16),
	RT_REG(RT9466_REG_CHG_ADC),
	RT_REG(RT9466_REG_CHG_CTRL19),
	RT_REG(RT9466_REG_CHG_CTRL17),
	RT_REG(RT9466_REG_CHG_CTRL18),
	RT_REG(RT9466_REG_DEVICE_ID),
	RT_REG(RT9466_REG_CHG_STAT),
	RT_REG(RT9466_REG_CHG_NTC),
	RT_REG(RT9466_REG_ADC_DATA_H),
	RT_REG(RT9466_REG_ADC_DATA_L),
	RT_REG(RT9466_REG_CHG_STATC),
	RT_REG(RT9466_REG_CHG_FAULT),
	RT_REG(RT9466_REG_TS_STATC),
	RT_REG(RT9466_REG_CHG_IRQ1),
	RT_REG(RT9466_REG_CHG_IRQ2),
	RT_REG(RT9466_REG_CHG_IRQ3),
	RT_REG(RT9466_REG_CHG_STATC_CTRL),
	RT_REG(RT9466_REG_CHG_FAULT_CTRL),
	RT_REG(RT9466_REG_TS_STATC_CTRL),
	RT_REG(RT9466_REG_CHG_IRQ1_CTRL),
	RT_REG(RT9466_REG_CHG_IRQ2_CTRL),
	RT_REG(RT9466_REG_CHG_IRQ3_CTRL),
};

#endif /* CONFIG_RT_REGMAP */

struct rt9466_desc {
	u32 ichg;	/* 10uA */
	u32 aicr;	/* 10uA */
	u32 mivr;	/* mV */
	u32 cv;		/* uV */
	u32 ieoc;	/* mA */
	u32 safety_timer;	/* hour */
	u32 ircmp_resistor;	/* mohm */
	u32 ircmp_vclamp;	/* mV */
	bool enable_te;
	bool enable_wdt;
	int regmap_represent_slave_addr;
	const char *regmap_name;
};

/* These default values will be used if there's no property in dts */
static struct rt9466_desc rt9466_default_desc = {
	.ichg = 200000,	/* 10uA */
	.aicr = 50000,	/* 10uA */
	.mivr = 4400,	/* mV */
	.cv = 4350000,	/* uA */
	.ieoc = 250,	/* mA */
	.safety_timer = 12,
#ifdef CONFIG_MTK_BIF_SUPPORT
	.ircmp_resistor = 0,
	.ircmp_vclamp = 0,
#else
	.ircmp_resistor = 80,
	.ircmp_vclamp = 224,
#endif
	.enable_te = true,
	.enable_wdt = true,
	.regmap_represent_slave_addr = RT9466_SLAVE_ADDR,
	.regmap_name = "rt9466",
};

struct rt9466_info {
	struct mtk_charger_info mchr_info;
	int i2c_log_level;
	struct i2c_client *i2c;
	struct mutex i2c_access_lock;
	struct mutex adc_access_lock;
	struct mutex aicr_access_lock;
	struct mutex ichg_access_lock;
	struct device *dev;
	struct rt9466_desc *desc;
	struct work_struct discharging_work;
	struct workqueue_struct *discharging_workqueue;
	bool err_state;
	int irq;
	u32 intr_gpio;
#ifdef CONFIG_RT_REGMAP
	struct rt_regmap_device *regmap_dev;
	struct rt_regmap_properties *regmap_prop;
#endif
};

/* ========================= */
/* I2C operations            */
/* ========================= */

static int rt9466_device_read(void *client, u32 addr, int leng, void *dst)
{
	int ret = 0;
	struct i2c_client *i2c = NULL;

	i2c = (struct i2c_client *)client;
	ret = i2c_smbus_read_i2c_block_data(i2c, addr, leng, dst);

	return ret;
}

static int rt9466_device_write(void *client, u32 addr, int leng,
	const void *src)
{
	int ret = 0;
	struct i2c_client *i2c = NULL;

	i2c = (struct i2c_client *)client;
	ret = i2c_smbus_write_i2c_block_data(i2c, addr, leng, src);

	return ret;
}

#ifdef CONFIG_RT_REGMAP
static struct rt_regmap_fops rt9466_regmap_fops = {
	.read_device = rt9466_device_read,
	.write_device = rt9466_device_write,
};

static int rt9466_register_rt_regmap(struct rt9466_info *info)
{
	int ret = 0;
	struct i2c_client *i2c = info->i2c;
	struct rt_regmap_properties *prop = NULL;

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	prop = devm_kzalloc(&i2c->dev, sizeof(struct rt_regmap_properties),
		GFP_KERNEL);
	if (!prop) {
		battery_log(BAT_LOG_CRTI, "%s: no enough memory\n", __func__);
		return -ENOMEM;
	}

	prop->name = info->desc->regmap_name;
	prop->aliases = info->desc->regmap_name;
	prop->register_num = ARRAY_SIZE(rt9466_regmap_map);
	prop->rm = rt9466_regmap_map;
	prop->rt_regmap_mode = RT_SINGLE_BYTE | RT_CACHE_DISABLE | RT_IO_PASS_THROUGH;
	prop->io_log_en = 0;

	info->regmap_prop = prop;
	info->regmap_dev = rt_regmap_device_register_ex(
		info->regmap_prop,
		&rt9466_regmap_fops,
		&i2c->dev,
		i2c,
		info->desc->regmap_represent_slave_addr,
		info
	);

	if (!info->regmap_dev) {
		dev_err(&i2c->dev, "register regmap device failed\n");
		return -EIO;
	}

	battery_log(BAT_LOG_CRTI, "%s: ends\n", __func__);

	return ret;
}
#endif /* CONFIG_RT_REGMAP */

static inline int _rt9466_i2c_write_byte(struct rt9466_info *info, u8 cmd,
	u8 data)
{
	int ret = 0, retry = 0;

	do {
#ifdef CONFIG_RT_REGMAP
		ret = rt_regmap_block_write(info->regmap_dev, cmd, 1, &data);
#else
		ret = rt9466_device_write(info->i2c, cmd, 1, &data);
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

static int rt9466_i2c_write_byte(struct rt9466_info *info, u8 cmd, u8 data)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = _rt9466_i2c_write_byte(info, cmd, data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}

static inline int _rt9466_i2c_read_byte(struct rt9466_info *info, u8 cmd)
{
	int ret = 0, ret_val = 0, retry = 0;

	do {
#ifdef CONFIG_RT_REGMAP
		ret = rt_regmap_block_read(info->regmap_dev, cmd, 1, &ret_val);
#else
		ret = rt9466_device_read(info->i2c, cmd, 1, &ret_val);
#endif
		retry++;
		if (ret < 0)
			msleep(20);
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

static int rt9466_i2c_read_byte(struct rt9466_info *info, u8 cmd)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = _rt9466_i2c_read_byte(info, cmd);
	mutex_unlock(&info->i2c_access_lock);

	if (ret < 0)
		return ret;

	return (ret & 0xFF);
}

static inline int _rt9466_i2c_block_write(struct rt9466_info *info, u8 cmd,
	u32 leng, const u8 *data)
{
	int ret = 0;

#ifdef CONFIG_RT_REGMAP
	ret = rt_regmap_block_write(info->regmap_dev, cmd, leng, data);
#else
	ret = rt9466_device_write(info->i2c, cmd, leng, data);
#endif

	return ret;
}


static int rt9466_i2c_block_write(struct rt9466_info *info, u8 cmd, u32 leng,
	const u8 *data)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = _rt9466_i2c_block_write(info, cmd, leng, data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}

static inline int _rt9466_i2c_block_read(struct rt9466_info *info, u8 cmd,
	u32 leng, u8 *data)
{
	int ret = 0;

#ifdef CONFIG_RT_REGMAP
	ret = rt_regmap_block_read(info->regmap_dev, cmd, leng, data);
#else
	ret = rt9466_device_read(info->i2c, cmd, leng, data);
#endif

	return ret;
}


static int rt9466_i2c_block_read(struct rt9466_info *info, u8 cmd, u32 leng,
	u8 *data)
{
	int ret = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = _rt9466_i2c_block_read(info, cmd, leng, data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}


static int rt9466_i2c_test_bit(struct rt9466_info *info, u8 cmd, u8 shift)
{
	int ret = 0;

	ret = rt9466_i2c_read_byte(info, cmd);
	if (ret < 0)
		return ret;

	ret = ret & (1 << shift);

	return ret;
}

static int rt9466_i2c_update_bits(struct rt9466_info *info, u8 cmd, u8 data,
	u8 mask)
{
	int ret = 0;
	u8 reg_data = 0;

	mutex_lock(&info->i2c_access_lock);
	ret = _rt9466_i2c_read_byte(info, cmd);
	if (ret < 0) {
		mutex_unlock(&info->i2c_access_lock);
		return ret;
	}

	reg_data = ret & 0xFF;
	reg_data &= ~mask;
	reg_data |= (data & mask);

	ret = _rt9466_i2c_write_byte(info, cmd, reg_data);
	mutex_unlock(&info->i2c_access_lock);

	return ret;
}

static inline int rt9466_set_bit(struct rt9466_info *info, u8 reg, u8 mask)
{
	return rt9466_i2c_update_bits(info, reg, mask, mask);
}

static inline int rt9466_clr_bit(struct rt9466_info *info, u8 reg, u8 mask)
{
	return rt9466_i2c_update_bits(info, reg, 0x00, mask);
}

/* ================== */
/* Internal Functions */
/* ================== */

/* The following APIs will be referenced in internal functions */
static int rt_charger_set_ichg(struct mtk_charger_info *mchr_info,
	void *data);
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
static int rt_charger_set_ircmp_resistor(struct mtk_charger_info *mchr_info,
	void *data);
static int rt_charger_set_ircmp_vclamp(struct mtk_charger_info *mchr_info,
	void *data);
static int rt_charger_is_power_path_enable(struct mtk_charger_info *mchr_info,
	void *data);

static u8 rt9466_find_closest_reg_value(const u32 min, const u32 max,
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

static u8 rt9466_find_closest_reg_value_via_table(const u32 *value_table,
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

static u32 rt9466_find_closest_real_value(const u32 min, const u32 max,
	const u32 step, const u8 reg_val)
{
	u32 ret_val = 0;

	ret_val = min + reg_val * step;
	if (ret_val > max)
		ret_val = max;

	return ret_val;
}

static irqreturn_t rt9466_irq_handler(int irq, void *data)
{
	int ret = 0;
	u8 irq_data[6] = {0};
	struct rt9466_info *info = (struct rt9466_info *)data;

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	ret = rt9466_i2c_block_read(info, RT9466_REG_CHG_STATC,
		ARRAY_SIZE(irq_data), irq_data);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: read irq failed\n", __func__);
		goto err_read_irq;
	}

	battery_log(BAT_LOG_CRTI,
		"%s: STATC = 0x%02X, FAULT = 0x%02X, TS = 0x%02X\n",
		__func__, irq_data[0], irq_data[1], irq_data[2]);
	battery_log(BAT_LOG_CRTI, "%s: IRQ1~3 = (0x%02X, 0x%02X, 0x%02X)\n",
		__func__, irq_data[3], irq_data[4], irq_data[5]);

err_read_irq:
	return IRQ_HANDLED;
}

static int rt9466_irq_register(struct rt9466_info *info)
{
	int ret = 0;

	ret = gpio_request_one(info->intr_gpio, GPIOF_IN, "rt9466_irq_gpio");
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: gpio request fail\n", __func__);
		return ret;
	}

	ret = gpio_to_irq(info->intr_gpio);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: irq mapping fail\n", __func__);
		goto err_to_irq;
	}
	info->irq = ret;
	battery_log(BAT_LOG_CRTI, "%s: irq = %d\n", __func__, info->irq);

	/* Request threaded IRQ */
	ret = devm_request_threaded_irq(info->dev, info->irq, NULL,
		rt9466_irq_handler, IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
		"rt9466_irq", info);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: request thread irq failed\n",
			__func__);
		goto err_request_irq;
	}

	return 0;

err_to_irq:
err_request_irq:
	gpio_free(info->intr_gpio);
	return ret;
}

static int rt9466_enable_all_irq(struct rt9466_info *info, bool enable)
{
	int ret = 0;

	battery_log(BAT_LOG_CRTI, "%s: enable = %d\n", __func__, enable);

	/* Enable/Disable all irq */
	if (enable)
		ret = rt9466_device_write(info->i2c, RT9466_REG_CHG_STATC_CTRL,
			ARRAY_SIZE(rt9466_irq_init_data), rt9466_irq_init_data);
	else
		ret = rt9466_device_write(info->i2c, RT9466_REG_CHG_STATC_CTRL,
			ARRAY_SIZE(rt9466_irq_maskall_data), rt9466_irq_maskall_data);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: %s irq failed\n",
			__func__, (enable ? "enable" : "disable"));

	return ret;
}

static int rt9466_irq_init(struct rt9466_info *info)
{
	int ret = 0;

	battery_log(BAT_LOG_CRTI, "%s\n", __func__);

	/* Disable all irq */
	ret = rt9466_enable_all_irq(info, false);

#if 0 /* Mask IRQs that are related to plug-in/out */
	ret = rt9466_i2c_block_write(info, RT9466_REG_CHG_STATC_CTRL,
		ARRAY_SIZE(rt9466_irq_init_data), rt9466_irq_init_data);
#endif
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: init irq failed\n", __func__);

	return ret;
}


static bool rt9466_is_hw_exist(struct rt9466_info *info)
{
	int ret = 0;
	u8 vendor_id = 0, chip_rev = 0;

	ret = i2c_smbus_read_byte_data(info->i2c, RT9466_REG_DEVICE_ID);
	if (ret < 0)
		return false;

	vendor_id = ret & 0xF0;
	chip_rev = ret & 0x0F;
	if (vendor_id != RT9466_VENDOR_ID) {
		battery_log(BAT_LOG_CRTI,
			"%s: vendor id is incorrect (0x%02X)\n",
			__func__, vendor_id);
		return false;
	}
	battery_log(BAT_LOG_CRTI, "%s: chip rev(E%d,0x%02X)\n",
		__func__, chip_rev, chip_rev);

	info->mchr_info.device_id = chip_rev;
	return true;
}

static int rt9466_set_fast_charge_timer(struct rt9466_info *info, u32 hour)
{
	int ret = 0;
	u8 reg_fct = 0;

	battery_log(BAT_LOG_CRTI, "%s: set fast charge timer to %d\n",
		__func__, hour);

	reg_fct = rt9466_find_closest_reg_value(RT9466_WT_FC_MIN, RT9466_WT_FC_MAX,
		RT9466_WT_FC_STEP, RT9466_WT_FC_NUM, hour);

	ret = rt9466_i2c_update_bits(
		info,
		RT9466_REG_CHG_CTRL12,
		reg_fct << RT9466_SHIFT_WT_FC,
		RT9466_MASK_WT_FC
	);

	return ret;
}

static int rt9466_enable_watchdog_timer(struct rt9466_info *info, bool enable)
{
	int ret = 0;

	ret = (enable ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL13, RT9466_MASK_WDT_EN);

	return ret;
}

/* Hardware pin current limit */
static int rt9466_enable_ilim(struct rt9466_info *info, bool enable)
{
	int ret = 0;

	battery_log(BAT_LOG_CRTI, "%s: enable ilim = %d\n", __func__, enable);

	ret = (enable ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL3, RT9466_MASK_ILIM_EN);

	return ret;
}

/* Select IINLMTSEL to use AICR */
static int rt9466_select_input_current_limit(struct rt9466_info *info,
	enum rt9466_iin_limit_sel sel)
{
	int ret = 0;

	battery_log(BAT_LOG_CRTI, "%s: select input current limit = %d\n",
		__func__, sel);

	ret = rt9466_i2c_update_bits(
		info,
		RT9466_REG_CHG_CTRL2,
		sel << RT9466_SHIFT_IINLMTSEL,
		RT9466_MASK_IINLMTSEL
	);

	return ret;
}

/* Enable/Disable hidden mode */
static int rt9466_enable_hidden_mode(struct rt9466_info *info, bool enable)
{
	int ret = 0;

	battery_log(BAT_LOG_CRTI, "%s: enable hidden mode = %d\n",
		__func__, enable);

	/* Disable hidden mode */
	if (!enable) {
		ret = rt9466_i2c_write_byte(info, 0x70, 0x00);
		if (ret < 0)
			goto err;
		return ret;
	}

	/* For platform before MT6755, len of block write cannnot over 8 byte */
	ret = rt9466_i2c_block_write(info,
		rt9466_reg_en_hidden_mode[0],
		ARRAY_SIZE(rt9466_val_en_hidden_mode) / 4,
		rt9466_val_en_hidden_mode);
	if (ret < 0)
		goto err;

	ret = rt9466_i2c_block_write(info,
		rt9466_reg_en_hidden_mode[4],
		ARRAY_SIZE(rt9466_val_en_hidden_mode) / 4,
		&(rt9466_val_en_hidden_mode[4]));
	if (ret < 0)
		goto err;

	return ret;

err:
	battery_log(BAT_LOG_CRTI, "%s: enable hidden mode = %d failed\n",
		__func__, enable);
	return ret;
}

static int rt9466_set_iprec(struct rt9466_info *info, u32 iprec)
{
	int ret = 0;
	u8 reg_iprec = 0;

	/* Find corresponding reg value */
	reg_iprec = rt9466_find_closest_reg_value(RT9466_IPREC_MIN,
		RT9466_IPREC_MAX, RT9466_IPREC_STEP, RT9466_IPREC_NUM, iprec);

	battery_log(BAT_LOG_CRTI, "%s: iprec = %d\n", __func__, iprec);

	ret = rt9466_i2c_update_bits(
		info,
		RT9466_REG_CHG_CTRL8,
		reg_iprec << RT9466_SHIFT_IPREC,
		RT9466_MASK_IPREC
	);

	return ret;
}

/* Software workaround */
static int rt9466_sw_workaround(struct rt9466_info *info)
{
	int ret = 0;
	u32 boot_mode = 0;

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	/* Enter hidden mode */
	ret = rt9466_enable_hidden_mode(info, true);
	if (ret < 0)
		goto out;

	/* Set precharge current to 850mA, only do this in normal boot */
	if (info->mchr_info.device_id <= RT9466_CHIP_REV_E3) {
		mtk_charger_get_platform_boot_mode(&info->mchr_info,
			&boot_mode);
		if (boot_mode == NORMAL_BOOT) {
			ret = rt9466_set_iprec(info, 850);
			if (ret < 0)
				goto out;

			/* Increase Isys drop threshold to 2.5A */
			ret = rt9466_i2c_write_byte(info, 0x26, 0x1C);
			if (ret < 0)
				goto out;
		}
	}

	/* Disable TS auto sensing */
	ret = rt9466_clr_bit(info, 0x2E, 0x01);
	if (ret < 0)
		goto out;

	/* Worst case delay: wait auto sensing */
	mdelay(200);

	/* Only revision <= E1 needs the following workaround */
	if (info->mchr_info.device_id > RT9466_CHIP_REV_E1)
		goto out;

	/* ICC: modify sensing node, make it more accurate */
	ret = rt9466_i2c_write_byte(info, 0x27, 0x00);
	if (ret < 0)
		goto out;

	/* DIMIN level */
	ret = rt9466_i2c_write_byte(info, 0x28, 0x86);

out:
	/* Exit hidden mode */
	ret = rt9466_enable_hidden_mode(info, false);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: exit hidden mode failed\n",
			__func__);

	return ret;
}

static int rt9466_get_adc(struct rt9466_info *info,
	enum rt9466_adc_sel adc_sel, int *adc_val)
{
	int ret = 0, i = 0;
	const int max_wait_retry = 5;
	u8 adc_data[2] = {0, 0};
	u32 aicr = 0, ichg = 0;

	mutex_lock(&info->adc_access_lock);
	info->i2c_log_level = BAT_LOG_CRTI;

	/* Select ADC to desired channel */
	ret = rt9466_i2c_update_bits(
		info,
		RT9466_REG_CHG_ADC,
		adc_sel << RT9466_SHIFT_ADC_IN_SEL,
		RT9466_MASK_ADC_IN_SEL
	);

	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: select ADC to %d failed\n",
			__func__, adc_sel);
		goto out;
	}

	/* Workaround for IBUS & IBAT */
	if (adc_sel == RT9466_ADC_IBUS) {
		mutex_lock(&info->aicr_access_lock);
		ret = rt_charger_get_aicr(&info->mchr_info, &aicr);
		if (ret < 0) {
			battery_log(BAT_LOG_CRTI, "%s: get_aicr failed\n",
				__func__);
			goto out;
		}
	} else if (adc_sel == RT9466_ADC_IBAT) {
		mutex_lock(&info->ichg_access_lock);
		ret = rt_charger_get_ichg(&info->mchr_info, &ichg);
		if (ret < 0) {
			battery_log(BAT_LOG_CRTI, "%s: get ichg failed\n",
				__func__);
			goto out;
		}
	}

	/* Start ADC conversation */
	ret = rt9466_set_bit(info, RT9466_REG_CHG_ADC, RT9466_MASK_ADC_START);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI,
			"%s: start ADC conversation failed, sel = %d\n",
			__func__, adc_sel);
		goto out;
	}

	/* Wait for ADC conversation */
	for (i = 0; i < max_wait_retry; i++) {
		mdelay(35);
		if (rt9466_i2c_test_bit(info, RT9466_REG_CHG_ADC,
			RT9466_SHIFT_ADC_START) == 0) {
			battery_log(info->i2c_log_level,
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
		goto out;
	}

	mdelay(1);

	/* Read ADC data high/low byte */
	ret = rt9466_i2c_block_read(info, RT9466_REG_ADC_DATA_H, 2, adc_data);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI,
			"%s: read ADC data failed\n", __func__);
		goto out;
	}

	/* Calculate ADC value */
	*adc_val = (adc_data[0] * 256 + adc_data[1]) * rt9466_adc_unit[adc_sel]
		+ rt9466_adc_offset[adc_sel];

	battery_log(info->i2c_log_level,
		"%s: adc_sel = %d, adc_h = 0x%02X, adc_l = 0x%02X, val = %d\n",
		__func__, adc_sel, adc_data[0], adc_data[1], *adc_val);


	ret = 0;

out:
	/* Coefficient of IBUS & IBAT */
	if (adc_sel == RT9466_ADC_IBUS) {
		if (aicr < 40000) /* 400mA */
			*adc_val = *adc_val * 67 / 100;
		mutex_unlock(&info->aicr_access_lock);
	} else if (adc_sel == RT9466_ADC_IBAT) {
		if (ichg >= 10000 && ichg <= 45000) /* 100~450mA */
			*adc_val = *adc_val * 57 / 100;
		else if (ichg >= 50000 && ichg <= 85000) /* 500~850mA */
			*adc_val = *adc_val * 63 / 100;
		mutex_unlock(&info->ichg_access_lock);
	}

	info->i2c_log_level = BAT_LOG_FULL;
	mutex_unlock(&info->adc_access_lock);
	return ret;
}

/* Reset all registers' value to default */
static int rt9466_reset_chip(struct rt9466_info *info)
{
	int ret = 0;

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	ret = rt9466_set_bit(info, RT9466_REG_CORE_CTRL0, RT9466_MASK_RST);

	return ret;
}

static int rt9466_enable_te(struct rt9466_info *info, const u8 enable)
{
	int ret = 0;

	ret = (enable ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL2, RT9466_MASK_TE_EN);

	return ret;
}

static int rt9466_set_ieoc(struct rt9466_info *info, const u32 ieoc)
{
	int ret = 0;

	/* Find corresponding reg value */
	u8 reg_ieoc = rt9466_find_closest_reg_value(RT9466_IEOC_MIN,
		RT9466_IEOC_MAX, RT9466_IEOC_STEP, RT9466_IEOC_NUM, ieoc);

	battery_log(BAT_LOG_CRTI, "%s: ieoc = %d\n", __func__, ieoc);

	ret = rt9466_i2c_update_bits(
		info,
		RT9466_REG_CHG_CTRL9,
		reg_ieoc << RT9466_SHIFT_IEOC,
		RT9466_MASK_IEOC
	);

	return ret;
}

static int rt9466_get_mivr(struct rt9466_info *info, u32 *mivr)
{
	int ret = 0;
	u8 reg_mivr = 0;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_CTRL6);
	if (ret < 0)
		return ret;
	reg_mivr = ((ret & RT9466_MASK_MIVR) >> RT9466_SHIFT_MIVR) & 0xFF;

	*mivr = rt9466_find_closest_real_value(RT9466_MIVR_MIN, RT9466_MIVR_MAX,
		RT9466_MIVR_STEP, reg_mivr);

	return ret;
}

static int rt9466_set_mivr(struct rt9466_info *info, u32 mivr)
{
	int ret = 0;
	u8 reg_mivr = 0;

	/* Find corresponding reg value */
	reg_mivr = rt9466_find_closest_reg_value(RT9466_MIVR_MIN,
		RT9466_MIVR_MAX, RT9466_MIVR_STEP, RT9466_MIVR_NUM, mivr);

	battery_log(BAT_LOG_CRTI, "%s: mivr = %d\n", __func__, mivr);

	ret = rt9466_i2c_update_bits(
		info,
		RT9466_REG_CHG_CTRL6,
		reg_mivr << RT9466_SHIFT_MIVR,
		RT9466_MASK_MIVR
	);

	return ret;
}

static int rt9466_enable_jeita(struct rt9466_info *info, bool enable)
{
	int ret = 0;

	ret = (enable ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL16, RT9466_MASK_JEITA_EN);

	return ret;
}


static int rt9466_init_setting(struct rt9466_info *info)
{
	int ret = 0;
	struct rt9466_desc *desc = info->desc;
	bool enable_safety_timer = true;

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	ret = rt9466_irq_init(info);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: irq init failed\n", __func__);

	/* Disable hardware ILIM */
	ret = rt9466_enable_ilim(info, false);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: disable ilim failed\n",
			__func__);

	/* Select IINLMTSEL to use AICR */
	ret = rt9466_select_input_current_limit(info, RT9466_IINLMTSEL_AICR);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: select iinlmtsel failed\n",
			__func__);

	ret = rt_charger_set_ichg(&info->mchr_info, &desc->ichg);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: set ichg failed\n", __func__);

	ret = rt_charger_set_aicr(&info->mchr_info, &desc->aicr);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: set aicr failed\n", __func__);

	ret = rt_charger_set_mivr(&info->mchr_info, &desc->mivr);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: set mivr failed\n", __func__);

	ret = rt_charger_set_battery_voreg(&info->mchr_info, &desc->cv);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: set voreg failed\n", __func__);

	ret = rt9466_set_ieoc(info, desc->ieoc);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: set ieoc failed\n", __func__);

	ret = rt9466_enable_te(info, desc->enable_te);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: set te failed\n", __func__);

	/* Set fast charge timer to 12 hours */
	ret = rt9466_set_fast_charge_timer(info, desc->safety_timer);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: set fast timer failed\n",
			__func__);

	ret = rt_charger_enable_safety_timer(&info->mchr_info,
		&enable_safety_timer);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: enable charger timer failed\n",
			__func__);

	ret = rt9466_enable_watchdog_timer(info, desc->enable_wdt);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: enable watchdog failed\n",
			__func__);

	ret = rt9466_enable_jeita(info, false);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: disable jeita failed\n",
			__func__);

	ret = rt_charger_set_ircmp_resistor(&info->mchr_info,
		&desc->ircmp_resistor);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI,
			"%s: set IR compensation resistor failed\n", __func__);

	ret = rt_charger_set_ircmp_vclamp(&info->mchr_info,
		&desc->ircmp_vclamp);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI,
			"%s: set IR compensation voltage clamp failed\n",
			__func__);

	return ret;
}


static int rt9466_get_charging_status(struct rt9466_info *info,
	enum rt9466_charging_status *chg_stat)
{
	int ret = 0;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_STAT);
	if (ret < 0)
		return ret;

	*chg_stat = (ret & RT9466_MASK_CHG_STAT) >> RT9466_SHIFT_CHG_STAT;

	return ret;
}

static int rt9466_get_ieoc(struct rt9466_info *info, u32 *ieoc)
{
	int ret = 0;
	u8 reg_ieoc = 0;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_CTRL9);
	if (ret < 0)
		return ret;

	reg_ieoc = (ret & RT9466_MASK_IEOC) >> RT9466_SHIFT_IEOC;
	*ieoc = rt9466_find_closest_real_value(RT9466_IEOC_MIN, RT9466_IEOC_MAX,
		RT9466_IEOC_STEP, reg_ieoc);

	return ret;
}

static int rt9466_is_charging_enable(struct rt9466_info *info, bool *enable)
{
	int ret = 0;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_CTRL2);
	if (ret < 0)
		return ret;

	*enable = ((ret & RT9466_MASK_CHG_EN) >> RT9466_SHIFT_CHG_EN) > 0 ?
		true : false;

	return ret;
}

static int rt9466_enable_pump_express(struct rt9466_info *info, bool enable)
{
	int ret = 0, i = 0;
	const int max_retry_times = 5;

	battery_log(BAT_LOG_CRTI, "%s: enable pumpX = %d\n", __func__, enable);

	ret = (enable ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL17, RT9466_MASK_PUMPX_EN);

	if (ret < 0)
		return ret;

	for (i = 0; i < max_retry_times; i++) {
		msleep(2500);
		if (rt9466_i2c_test_bit(info, RT9466_REG_CHG_CTRL17,
			RT9466_SHIFT_PUMPX_EN) == 0)
			break;
	}

	if (i == max_retry_times) {
		battery_log(BAT_LOG_CRTI, "%s: pumpX wait failed\n",
			__func__);
		ret = -EIO;
	}

	return ret;
}

static int rt9466_set_iin_vth(struct rt9466_info *info, u32 iin_vth)
{
	int ret = 0;
	u8 reg_iin_vth = 0;

	battery_log(BAT_LOG_CRTI, "%s: set iin_vth = %d\n", __func__, iin_vth);

	reg_iin_vth = rt9466_find_closest_reg_value(RT9466_IIN_VTH_MIN,
		RT9466_IIN_VTH_MAX, RT9466_IIN_VTH_STEP, RT9466_IIN_VTH_NUM,
		iin_vth);

	ret = rt9466_i2c_update_bits(
		info,
		RT9466_REG_CHG_CTRL14,
		reg_iin_vth << RT9466_SHIFT_IIN_VTH,
		RT9466_MASK_IIN_VTH
	);

	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: set iin vth failed, ret = %d\n",
			__func__, ret);

	return ret;
}

static void rt9466_discharging_work_handler(struct work_struct *work)
{
	int ret = 0, i = 0;
	const unsigned int max_retry_cnt = 3;
	struct rt9466_info *info = (struct rt9466_info *)container_of(work,
		struct rt9466_info, discharging_work);

	ret = rt9466_enable_hidden_mode(info, true);
	if (ret < 0)
		return;

	battery_log(BAT_LOG_CRTI, "%s: start discharging\n", __func__);

	/* Set bit2 of reg[0x21] to 1 to enable discharging */
	ret = rt9466_set_bit(info, 0x21, 0x04);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: enable discharging failed\n",
			__func__);
		goto out;
	}

	/* Wait for discharging to 0V */
	mdelay(20);

	for (i = 0; i < max_retry_cnt; i++) {
		/* Disable discharging */
		ret = rt9466_clr_bit(info, 0x21, 0x04);
		if (ret < 0)
			continue;
		if (rt9466_i2c_test_bit(info, 0x21, 2) == 0)
			goto out;
	}

	battery_log(BAT_LOG_CRTI, "%s: disable discharging failed\n",
		__func__);
out:
	rt9466_enable_hidden_mode(info, false);
	battery_log(BAT_LOG_CRTI, "%s: end discharging\n", __func__);
}

static int rt9466_run_discharging(struct rt9466_info *info)
{
	if (!queue_work(info->discharging_workqueue,
		&info->discharging_work))
		return -EINVAL;

	return 0;
}

static int rt9466_parse_dt(struct rt9466_info *info, struct device *dev)
{
	int ret = 0;
	struct rt9466_desc *desc = NULL;
	struct device_node *np = dev->of_node;

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	if (!np) {
		battery_log(BAT_LOG_CRTI, "%s: no device node\n", __func__);
		return -EINVAL;
	}

	info->desc = &rt9466_default_desc;

	desc = devm_kzalloc(dev, sizeof(struct rt9466_desc), GFP_KERNEL);
	if (!desc) {
		battery_log(BAT_LOG_CRTI, "%s: no enough memory\n", __func__);
		return -ENOMEM;
	}
	memcpy(desc, &rt9466_default_desc, sizeof(struct rt9466_desc));

	if (of_property_read_string(np, "charger_name",
		&(info->mchr_info.name)) < 0) {
		battery_log(BAT_LOG_CRTI, "%s: no charger name\n", __func__);
		info->mchr_info.name = "primary_charger";
	}

#if (!defined(CONFIG_MTK_GPIO) || defined(CONFIG_MTK_GPIOLIB_STAND))
	ret = of_get_named_gpio(np, "rt,intr_gpio", 0);
	if (ret < 0)
		return ret;
	info->intr_gpio = ret;
#else
	ret = of_property_read_u32(np, "rt,intr_gpio_num", &info->intr_gpio);
	if (ret < 0)
		return ret;
#endif
	battery_log(BAT_LOG_CRTI, "%s: intr gpio = %d\n", __func__,
		info->intr_gpio);

	if (of_property_read_u32(np, "regmap_represent_slave_addr",
		&desc->regmap_represent_slave_addr) < 0)
		battery_log(BAT_LOG_CRTI, "%s: no regmap slave addr\n",
			__func__);

	if (of_property_read_string(np, "regmap_name",
		&(desc->regmap_name)) < 0)
		battery_log(BAT_LOG_CRTI, "%s: no regmap name\n", __func__);

	if (of_property_read_u32(np, "ichg", &desc->ichg) < 0)
		battery_log(BAT_LOG_CRTI, "%s: no ichg\n", __func__);

	if (of_property_read_u32(np, "aicr", &desc->aicr) < 0)
		battery_log(BAT_LOG_CRTI, "%s: no aicr\n", __func__);

	if (of_property_read_u32(np, "mivr", &desc->mivr) < 0)
		battery_log(BAT_LOG_CRTI, "%s: no mivr\n", __func__);

	if (of_property_read_u32(np, "cv", &desc->cv) < 0)
		battery_log(BAT_LOG_CRTI, "%s: no cv\n", __func__);

	if (of_property_read_u32(np, "ieoc", &desc->ieoc) < 0)
		battery_log(BAT_LOG_CRTI, "%s: no ieoc\n", __func__);

	if (of_property_read_u32(np, "safety_timer", &desc->safety_timer) < 0)
		battery_log(BAT_LOG_CRTI, "%s: no safety timer\n", __func__);

	if (of_property_read_u32(np, "ircmp_resistor",
		&desc->ircmp_resistor) < 0)
		battery_log(BAT_LOG_CRTI, "%s: no ircmp resistor\n", __func__);

	if (of_property_read_u32(np, "ircmp_vclamp", &desc->ircmp_vclamp) < 0)
		battery_log(BAT_LOG_CRTI, "%s: no ircmp vclamp\n", __func__);

	desc->enable_te = of_property_read_bool(np, "enable_te");
	desc->enable_wdt = of_property_read_bool(np, "enable_wdt");

	info->desc = desc;

	return 0;
}

/* =========================================================== */
/* The following is implementation for interface of rt_charger */
/* =========================================================== */

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
	enum rt9466_charging_status chg_status = RT9466_CHG_STATUS_READY;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	ret = rt_charger_get_ichg(mchr_info, &ichg);
	ret = rt_charger_get_aicr(mchr_info, &aicr);
	ret = rt9466_get_mivr(info, &mivr);
	ret = rt9466_is_charging_enable(info, &chg_enable);
	ret = rt9466_get_ieoc(info, &ieoc);
	ret = rt9466_get_charging_status(info, &chg_status);
	ret = rt9466_get_adc(info, RT9466_ADC_VSYS, &adc_vsys);
	ret = rt9466_get_adc(info, RT9466_ADC_VBAT, &adc_vbat);
	ret = rt9466_get_adc(info, RT9466_ADC_IBAT, &adc_ibat);

	/*
	 * Log level is greater than critical or charging fault
	 * Dump all registers' value
	 */
	if ((Enable_BATDRV_LOG > BAT_LOG_CRTI) ||
	    (chg_status == RT9466_CHG_STATUS_FAULT)) {
		info->i2c_log_level = BAT_LOG_CRTI;
		for (i = 0; i < ARRAY_SIZE(rt9466_reg_addr); i++) {
			ret = rt9466_i2c_read_byte(info, rt9466_reg_addr[i]);
			if (ret < 0)
				return ret;
		}
	} else
		info->i2c_log_level = BAT_LOG_FULL;

	battery_log(BAT_LOG_CRTI,
		"%s: ICHG = %dmA, AICR = %dmA, MIVR = %dmV, IEOC = %dmA\n",
		__func__, ichg / 100, aicr / 100, mivr, ieoc);

	battery_log(BAT_LOG_CRTI,
		"%s: VSYS = %dmV, VBAT = %dmV, IBAT = %dmA\n",
		__func__, adc_vsys, adc_vbat, adc_ibat);

	battery_log(BAT_LOG_CRTI,
		"%s: CHG_EN = %d, CHG_STATUS = %s\n",
		__func__, chg_enable, rt9466_chg_status_name[chg_status]);

	return ret;
}

static int rt_charger_enable_charging(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u8 enable = *((u8 *)data);
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	ret = (enable ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL2, RT9466_MASK_CHG_EN);

	return ret;
}

static int rt_charger_enable_hz(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u8 enable = *((u8 *)data);
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	battery_log(BAT_LOG_CRTI, "%s: enable = %d\n", __func__, enable);
	ret = (enable ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL1, RT9466_MASK_HZ_EN);

	return ret;
}

static int rt_charger_enable_safety_timer(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u8 enable = *((u8 *)data);
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	battery_log(BAT_LOG_CRTI, "%s: enable = %d\n", __func__, enable);
	ret = (enable ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL12, RT9466_MASK_TMR_EN);

	return ret;
}

static int rt_charger_enable_otg(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0, i = 0;
	const int ss_wait_times = 5; /* soft start wait times */
	u8 enable = *((u8 *)data);
	u32 current_limit = 500;
	int vbus = 0;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	/* Set OTG_OC to 500mA */
	ret = rt_charger_set_boost_current_limit(mchr_info, &current_limit);
	if (ret < 0)
		return ret;

	/* Switch OPA mode to boost mode */
	battery_log(BAT_LOG_CRTI, "%s: otg enable = %d\n", __func__, enable);
	ret = (enable ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL1, RT9466_MASK_OPA_MODE);

	if (!enable) {
		/* Enter hidden mode */
		ret = rt9466_enable_hidden_mode(info, true);
		if (ret < 0)
			return ret;

		/* Workaround: reg[0x25] = 0x0F after leaving OTG mode */
		ret = rt9466_i2c_write_byte(info, 0x25, 0x0F);
		if (ret < 0)
			battery_log(BAT_LOG_CRTI, "%s: otg workaroud failed\n",
				__func__);

		/* Exit hidden mode */
		ret = rt9466_enable_hidden_mode(info, false);

		ret = rt9466_run_discharging(info);
		if (ret < 0)
			battery_log(BAT_LOG_CRTI, "%s: run discharging failed\n",
				__func__);
		return ret;
	}

	/* Wait for soft start finish interrupt */
	for (i = 0; i < ss_wait_times; i++) {
		ret = rt9466_get_adc(info, RT9466_ADC_VBUS_DIV2, &vbus);
		battery_log(BAT_LOG_CRTI, "%s: vbus = %dmV\n", __func__, vbus);
		if (ret == 0 && vbus > 4000) {
			/* Enter hidden mode */
			ret = rt9466_enable_hidden_mode(info, true);
			if (ret < 0)
				return ret;

			/* Woraround reg[0x25] = 0x00 after entering OTG mode */
			ret = rt9466_i2c_write_byte(info, 0x25, 0x00);
			if (ret < 0)
				battery_log(BAT_LOG_CRTI,
					"%s: otg workaroud failed\n", __func__);

			/* Exit hidden mode */
			ret = rt9466_enable_hidden_mode(info, false);
			battery_log(BAT_LOG_FULL,
				"%s: otg soft start successfully\n", __func__);
			break;
		}
	}
	if (i == ss_wait_times) {
		battery_log(BAT_LOG_CRTI,
			"%s: otg soft start failed\n", __func__);
		/* Disable OTG */
		rt9466_clr_bit(info, RT9466_REG_CHG_CTRL1,
			RT9466_MASK_OPA_MODE);
		ret = -EIO;
	}

	return ret;
}

static int rt_charger_enable_power_path(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u8 enable = *((u8 *)data);
	u32 mivr = (enable ? 4500 : RT9466_MIVR_MAX);
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	battery_log(BAT_LOG_CRTI, "%s: enable = %d\n", __func__, enable);
	ret = rt9466_set_mivr(info, mivr);

	return ret;
}

static int rt_charger_set_ichg(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u8 reg_ichg = 0;
	u32 ichg = *((u32 *)data);
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	/* This lock is for ADC IBAT */
	mutex_lock(&info->ichg_access_lock);

	/* MTK's current unit : 10uA */
	/* Our current unit : mA */
	ichg /= 100;

	/* Find corresponding reg value */
	reg_ichg = rt9466_find_closest_reg_value(RT9466_ICHG_MIN,
		RT9466_ICHG_MAX, RT9466_ICHG_STEP, RT9466_ICHG_NUM, ichg);

	battery_log(BAT_LOG_CRTI, "%s: ichg = %d\n", __func__, ichg);

	ret = rt9466_i2c_update_bits(
		info,
		RT9466_REG_CHG_CTRL7,
		reg_ichg << RT9466_SHIFT_ICHG,
		RT9466_MASK_ICHG
	);

	mutex_unlock(&info->ichg_access_lock);
	return ret;
}

static int rt_charger_set_aicr(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u8 reg_aicr = 0;
	u32 aicr = *((u32 *)data);
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	/* This lock is for ADC's IBUS */
	mutex_lock(&info->aicr_access_lock);

	/* MTK's current unit : 10uA */
	/* Our current unit : mA */
	aicr /= 100;

	/* Find corresponding reg value */
	reg_aicr = rt9466_find_closest_reg_value(RT9466_AICR_MIN,
		RT9466_AICR_MAX, RT9466_AICR_STEP, RT9466_AICR_NUM, aicr);

	battery_log(BAT_LOG_CRTI, "%s: aicr = %d\n", __func__, aicr);

	ret = rt9466_i2c_update_bits(
		info,
		RT9466_REG_CHG_CTRL3,
		reg_aicr << RT9466_SHIFT_AICR,
		RT9466_MASK_AICR
	);

	mutex_unlock(&info->aicr_access_lock);
	return ret;
}

static int rt_charger_set_mivr(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u32 mivr = *((u32 *)data);
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;
	bool enable = true;

	ret = rt_charger_is_power_path_enable(mchr_info, &enable);
	if (!enable) {
		battery_log(BAT_LOG_CRTI,
			"%s: power path is disabled, op is not allowed\n",
			__func__);
		return -EINVAL;
	}

	ret = rt9466_set_mivr(info, mivr);

	return ret;
}

static int rt_charger_set_battery_voreg(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u8 reg_voreg = 0;
	u32 voreg = *((u32 *)data);
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	/* MTK's voltage unit : uV */
	/* Our voltage unit : mV */
	voreg /= 1000;

	reg_voreg = rt9466_find_closest_reg_value(RT9466_BAT_VOREG_MIN,
		RT9466_BAT_VOREG_MAX, RT9466_BAT_VOREG_STEP,
		RT9466_BAT_VOREG_NUM, voreg);

	battery_log(BAT_LOG_CRTI, "%s: bat voreg = %d\n", __func__, voreg);

	ret = rt9466_i2c_update_bits(
		info,
		RT9466_REG_CHG_CTRL4,
		reg_voreg << RT9466_SHIFT_BAT_VOREG,
		RT9466_MASK_BAT_VOREG
	);

	return ret;
}

static int rt_charger_set_boost_current_limit(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u8 reg_ilimit = 0;
	u32 current_limit = *((u32 *)data);
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	reg_ilimit = rt9466_find_closest_reg_value_via_table(
		rt9466_boost_oc_threshold,
		ARRAY_SIZE(rt9466_boost_oc_threshold),
		current_limit
	);

	battery_log(BAT_LOG_CRTI, "%s: boost ilimit = %d\n",
		__func__, current_limit);

	ret = rt9466_i2c_update_bits(
		info,
		RT9466_REG_CHG_CTRL10,
		reg_ilimit << RT9466_SHIFT_BOOST_OC,
		RT9466_MASK_BOOST_OC
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
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	battery_log(BAT_LOG_CRTI, "%s: pe1.0 pump_up = %d\n",
		__func__, is_pump_up);

	/* Set to PE1.0 */
	ret = rt9466_clr_bit(info, RT9466_REG_CHG_CTRL17,
		RT9466_MASK_PUMPX_20_10);

	/* Set Pump Up/Down */
	ret = (is_pump_up ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL17, RT9466_MASK_PUMPX_UP_DN);

	ret = rt_charger_set_aicr(mchr_info, &aicr);
	if (ret < 0)
		return ret;

	ret = rt_charger_set_ichg(mchr_info, &ichg);
	if (ret < 0)
		return ret;

	/* Enable PumpX */
	ret = rt9466_enable_pump_express(info, true);

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

	msleep(250);

	aicr = 70000;
	ret = rt_charger_set_aicr(mchr_info, &aicr);
	return ret;
}

int rt_charger_set_pep20_current_pattern(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u8 reg_volt = 0;
	u32 voltage = *((u32 *)data);
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	battery_log(BAT_LOG_CRTI, "%s: pep2.0  = %d\n", __func__, voltage);

	/* Find register value of target voltage */
	reg_volt = rt9466_find_closest_reg_value(RT9466_PEP20_VOLT_MIN,
		RT9466_PEP20_VOLT_MAX, RT9466_PEP20_VOLT_STEP,
		RT9466_PEP20_VOLT_NUM, voltage);

	/* Set to PEP2.0 */
	ret = rt9466_set_bit(info, RT9466_REG_CHG_CTRL17,
		RT9466_MASK_PUMPX_20_10);
	if (ret < 0)
		return ret;

	/* Set Voltage */
	ret = rt9466_i2c_update_bits(
		info,
		RT9466_REG_CHG_CTRL17,
		reg_volt << RT9466_SHIFT_PUMPX_DEC,
		RT9466_MASK_PUMPX_DEC
	);
	if (ret < 0)
		return ret;

	/* Enable PumpX */
	ret = rt9466_enable_pump_express(info, true);

	return ret;
}

static int rt_charger_get_ichg(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u8 reg_ichg = 0;
	u32 ichg = 0;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_CTRL7);
	if (ret < 0)
		return ret;

	reg_ichg = (ret & RT9466_MASK_ICHG) >> RT9466_SHIFT_ICHG;
	ichg = rt9466_find_closest_real_value(RT9466_ICHG_MIN, RT9466_ICHG_MAX,
		RT9466_ICHG_STEP, reg_ichg);

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
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_CTRL3);
	if (ret < 0)
		return ret;

	reg_aicr = (ret & RT9466_MASK_AICR) >> RT9466_SHIFT_AICR;
	aicr = rt9466_find_closest_real_value(RT9466_AICR_MIN, RT9466_AICR_MAX,
		RT9466_AICR_STEP, reg_aicr);

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
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	/* Get value from ADC */
	ret = rt9466_get_adc(info, RT9466_ADC_TEMP_JC, &adc_temp);
	if (ret < 0)
		return ret;

	((int *)data)[0] = adc_temp;
	((int *)data)[1] = adc_temp;

	battery_log(BAT_LOG_CRTI, "%s: temperature = %d\n", __func__, adc_temp);
	return ret;
}

static int rt_charger_get_vbus(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0, adc_vbus = 0;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	/* Get value from ADC */
	ret = rt9466_get_adc(info, RT9466_ADC_VBUS_DIV2, &adc_vbus);
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
	enum rt9466_charging_status chg_stat = RT9466_CHG_STATUS_READY;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	ret = rt9466_get_charging_status(info, &chg_stat);

	/* Return is charging done or not */
	switch (chg_stat) {
	case RT9466_CHG_STATUS_READY:
	case RT9466_CHG_STATUS_PROGRESS:
	case RT9466_CHG_STATUS_FAULT:
		*((u32 *)data) = 0;
		break;
	case RT9466_CHG_STATUS_DONE:
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
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;
	u32 mivr = 0;

	ret = rt9466_get_mivr(info, &mivr);
	if (ret < 0)
		return ret;

	*((bool *)data) = ((mivr == RT9466_MIVR_MAX) ? false : true);


	return ret;
}

static int rt_charger_is_safety_timer_enable(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	ret = rt9466_i2c_test_bit(info, RT9466_REG_CHG_CTRL12,
		RT9466_SHIFT_TMR_EN);
	if (ret < 0)
		return ret;

	*((bool *)data) = (ret > 0) ? true : false;

	return ret;
}

static int rt_charger_reset_watchdog_timer(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	enum rt9466_charging_status chg_status = RT9466_CHG_STATUS_READY;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	/* Any I2C communication can reset watchdog timer */
	ret = rt9466_get_charging_status(info, &chg_status);

	return ret;
}

static int rt_charger_run_aicl(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0, i = 0;
	u32 mivr = 0, iin_vth = 0, aicr = 0;
	const u32 max_wait_time = 5;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	/* Check whether MIVR loop is active */
	if (rt9466_i2c_test_bit(info, RT9466_REG_CHG_STATC,
		RT9466_SHIFT_CHG_MIVR) <= 0) {
		battery_log(BAT_LOG_CRTI, "%s: mivr loop is not active\n",
			__func__);
		return 0;
	}

	battery_log(BAT_LOG_CRTI, "%s: mivr loop is active\n", __func__);
	ret = rt9466_get_mivr(info, &mivr);
	if (ret < 0)
		goto out;

	/* Check if there's a suitable IIN_VTH */
	iin_vth = mivr + 200;
	if (iin_vth > RT9466_IIN_VTH_MAX) {
		battery_log(BAT_LOG_CRTI, "%s: no suitable IIN_VTH, vth = %d\n",
			__func__, iin_vth);
		ret = -EINVAL;
		goto out;
	}

	ret = rt9466_set_iin_vth(info, iin_vth);
	if (ret < 0)
		goto out;

	ret = rt9466_set_bit(info, RT9466_REG_CHG_CTRL14, RT9466_MASK_IIN_MEAS);
	if (ret < 0)
		goto out;

	for (i = 0; i < max_wait_time; i++) {
		msleep(500);
		if (rt9466_i2c_test_bit(info, RT9466_REG_CHG_CTRL14,
			RT9466_SHIFT_IIN_MEAS) == 0)
			break;
	}
	if (i == max_wait_time) {
		battery_log(BAT_LOG_CRTI, "%s: wait AICL time out\n", __func__);
		ret = -EIO;
		goto out;
	}

	ret = rt_charger_get_aicr(mchr_info, &aicr);
	if (ret < 0)
		goto out;

	*((u32 *)data) = aicr / 100;
	battery_log(BAT_LOG_CRTI, "%s: OK, aicr upper bound = %dmA\n",
		__func__, aicr / 100);

	return 0;

out:
	*((u32 *)data) = 0;
	return ret;
}

static int rt_charger_set_ircmp_resistor(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u32 resistor = 0;
	u8 reg_resistor = 0;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	resistor = *((u32 *)data);
	battery_log(BAT_LOG_CRTI, "%s: set ir comp resistor = %d\n",
		__func__, resistor);

	reg_resistor = rt9466_find_closest_reg_value(RT9466_IRCMP_RES_MIN,
		RT9466_IRCMP_RES_MAX, RT9466_IRCMP_RES_STEP,
		RT9466_IRCMP_RES_NUM, resistor);

	ret = rt9466_i2c_update_bits(
		info,
		RT9466_REG_CHG_CTRL18,
		reg_resistor << RT9466_SHIFT_IRCMP_RES,
		RT9466_MASK_IRCMP_RES
	);

	return ret;
}

static int rt_charger_set_ircmp_vclamp(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u32 vclamp = 0;
	u8 reg_vclamp = 0;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	vclamp = *((u32 *)data);
	battery_log(BAT_LOG_CRTI, "%s: set ir comp vclamp = %d\n",
		__func__, vclamp);

	reg_vclamp = rt9466_find_closest_reg_value(RT9466_IRCMP_VCLAMP_MIN,
		RT9466_IRCMP_VCLAMP_MAX, RT9466_IRCMP_VCLAMP_STEP,
		RT9466_IRCMP_VCLAMP_NUM, vclamp);

	ret = rt9466_i2c_update_bits(
		info,
		RT9466_REG_CHG_CTRL18,
		reg_vclamp << RT9466_SHIFT_IRCMP_VCLAMP,
		RT9466_MASK_IRCMP_VCLAMP
	);

	return ret;
}

#if 0 /* Uncomment this block if these interfaces are released at your platform */
static int rt_charger_set_pep20_efficiency_table(
	struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	pep20_profile_t *profile = (pep20_profile_t *)data;

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
#endif

static int rt_charger_set_error_state(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	info->err_state = *((bool *)data);
	ret = rt_charger_enable_hz(mchr_info, &info->err_state);

	return ret;
}
/*
 * This function is used in shutdown function
 * Use i2c smbus directly
 */
static int rt_charger_sw_reset(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u8 irq_data[5] = {0};
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	/* Register 0x01 ~ 0x10 */
	const u8 reg_data[] = {
		0x10, 0x03, 0x23, 0x3C, 0x67, 0x0B, 0x4C, 0xA1,
		0x3C, 0x58, 0x2C, 0x02, 0x52, 0x05, 0x00, 0x10
	};

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);
	info->i2c_log_level = BAT_LOG_CRTI;

	/* Mask all irq */
	ret = rt9466_enable_all_irq(info, false);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: mask all irq failed\n",
			__func__);

	/* Read all irq */
	ret = rt9466_device_read(info->i2c, RT9466_REG_CHG_STATC,
		ARRAY_SIZE(irq_data), irq_data);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: read all irq failed\n",
			__func__);

	/* Reset necessary registers */
	ret = rt9466_device_write(info->i2c, RT9466_REG_CHG_CTRL1,
		ARRAY_SIZE(reg_data), reg_data);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: reset registers failed\n",
			__func__);

	info->i2c_log_level = BAT_LOG_FULL;
	return ret;
}


static const mtk_charger_intf rt9466_mchr_intf[CHARGING_CMD_NUMBER] = {
	[CHARGING_CMD_INIT] = rt_charger_hw_init,
	[CHARGING_CMD_DUMP_REGISTER] = rt_charger_dump_register,
	[CHARGING_CMD_ENABLE] = rt_charger_enable_charging,
	[CHARGING_CMD_SET_HIZ_SWCHR] = rt_charger_enable_hz,
	[CHARGING_CMD_ENABLE_SAFETY_TIMER] = rt_charger_enable_safety_timer,
	[CHARGING_CMD_ENABLE_OTG] = rt_charger_enable_otg,
	[CHARGING_CMD_ENABLE_POWER_PATH] = rt_charger_enable_power_path,
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
	[CHARGING_CMD_GET_VBUS] = rt_charger_get_vbus,
	[CHARGING_CMD_RUN_AICL] = rt_charger_run_aicl,
	[CHARGING_CMD_SET_IRCMP_RESISTOR] = rt_charger_set_ircmp_resistor,
	[CHARGING_CMD_SET_IRCMP_VOLT_CLAMP] = rt_charger_set_ircmp_vclamp,
#if 0 /* Uncomment this block if these interfaces are released at your platform */
	[CHARGING_CMD_SET_PEP20_EFFICIENCY_TABLE] = rt_charger_set_pep20_efficiency_table,
	[CHARGING_CMD_SW_RESET] = rt_charger_sw_reset,
#endif

	/*
	 * The following interfaces are not related to charger
	 * Define in mtk_charger_intf.c
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
/* I2C driver function       */
/* ========================= */

static int rt9466_probe(struct i2c_client *i2c,
	const struct i2c_device_id *dev_id)
{
	int ret = 0;
	struct rt9466_info *info = NULL;

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	info = devm_kzalloc(&i2c->dev, sizeof(struct rt9466_info), GFP_KERNEL);
	if (!info) {
		battery_log(BAT_LOG_CRTI, "%s: no enough memory\n", __func__);
		return -ENOMEM;
	}
	info->i2c = i2c;
	info->i2c_log_level = BAT_LOG_FULL;
	info->dev = &i2c->dev;
	mutex_init(&info->i2c_access_lock);
	mutex_init(&info->adc_access_lock);
	mutex_init(&info->ichg_access_lock);
	mutex_init(&info->aicr_access_lock);

	/* Is HW exist */
	if (!rt9466_is_hw_exist(info)) {
		battery_log(BAT_LOG_CRTI, "%s: no rt9466 exists\n", __func__);
		ret = -ENODEV;
		goto err_no_dev;
	}
	i2c_set_clientdata(i2c, info);

	ret = rt9466_parse_dt(info, &i2c->dev);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: parse dt failed\n", __func__);
		goto err_parse_dt;
	}

	ret = rt9466_irq_register(info);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: irq init failed\n", __func__);
		goto err_irq_register;
	}

#ifdef CONFIG_RT_REGMAP
	ret = rt9466_register_rt_regmap(info);
	if (ret < 0)
		goto err_register_regmap;
#endif

	/* Reset chip */
	ret = rt9466_reset_chip(info);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI,
			"%s: set register to default value failed\n", __func__);

	ret = rt9466_init_setting(info);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: set failed\n", __func__);
		goto err_init_setting;
	}

	ret = rt9466_sw_workaround(info);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI,
			"%s: software workaround failed\n", __func__);
		goto err_sw_workaround;
	}

	/* Create single thread workqueue */
	info->discharging_workqueue =
		create_singlethread_workqueue("discharging_workqueue");
	if (!info->discharging_workqueue) {
		ret = -ENOMEM;
		goto err_create_wq;
	}

	/* Init discharging workqueue */
	INIT_WORK(&info->discharging_work,
		rt9466_discharging_work_handler);

	rt_charger_dump_register(&info->mchr_info, NULL);

	/* Hook chr_control_interface with battery's interface */
	info->mchr_info.mchr_intf = rt9466_mchr_intf;
	mtk_charger_set_info(&info->mchr_info);
	battery_log(BAT_LOG_CRTI, "%s: ends\n", __func__);

	return ret;

err_init_setting:
err_sw_workaround:
err_create_wq:
#ifdef CONFIG_RT_REGMAP
	rt_regmap_device_unregister(info->regmap_dev);
err_register_regmap:
#endif
err_no_dev:
err_irq_register:
err_parse_dt:
	mutex_destroy(&info->i2c_access_lock);
	mutex_destroy(&info->adc_access_lock);
	mutex_destroy(&info->ichg_access_lock);
	mutex_destroy(&info->aicr_access_lock);
	return ret;
}


static int rt9466_remove(struct i2c_client *i2c)
{
	int ret = 0;
	struct rt9466_info *info = i2c_get_clientdata(i2c);

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	if (info) {
		if (info->discharging_workqueue)
			destroy_workqueue(info->discharging_workqueue);
#ifdef CONFIG_RT_REGMAP
		rt_regmap_device_unregister(info->regmap_dev);
#endif
		mutex_destroy(&info->i2c_access_lock);
		mutex_destroy(&info->adc_access_lock);
		mutex_destroy(&info->ichg_access_lock);
		mutex_destroy(&info->aicr_access_lock);
	}

	return ret;
}

static void rt9466_shutdown(struct i2c_client *i2c)
{
	int ret = 0;
	struct rt9466_info *info = i2c_get_clientdata(i2c);

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);
	if (info) {
		ret = rt_charger_sw_reset(&info->mchr_info, NULL);
		if (ret < 0)
			battery_log(BAT_LOG_CRTI,
				"%s: sw reset failed\n", __func__);
		info->i2c_log_level = BAT_LOG_CRTI;
	}
}

static const struct i2c_device_id rt9466_i2c_id[] = {
	{"rt9466", 0},
	{}
};

#ifdef CONFIG_OF
static const struct of_device_id rt9466_of_match[] = {
	{ .compatible = "richtek,rt9466", },
	{},
};
#else /* Not define CONFIG_OF */

#define RT9466_BUSNUM 1

static struct i2c_board_info rt9466_i2c_board_info __initdata = {
	I2C_BOARD_INFO("rt9466", RT9466_SALVE_ADDR)
};
#endif /* CONFIG_OF */


static struct i2c_driver rt9466_i2c_driver = {
	.driver = {
		.name = "rt9466",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = rt9466_of_match,
#endif
	},
	.probe = rt9466_probe,
	.remove = rt9466_remove,
	.shutdown = rt9466_shutdown,
	.id_table = rt9466_i2c_id,
};

static int __init rt9466_init(void)
{
	int ret = 0;

#ifdef CONFIG_OF
	battery_log(BAT_LOG_CRTI, "%s: with dts\n", __func__);
#else
	battery_log(BAT_LOG_CRTI, "%s: without dts\n", __func__);
	i2c_register_board_info(RT9466_BUSNUM, &rt9466_i2c_board_info, 1);
#endif

	ret = i2c_add_driver(&rt9466_i2c_driver);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: register i2c driver failed\n",
			__func__);

	return ret;
}
module_init(rt9466_init);


static void __exit rt9466_exit(void)
{
	i2c_del_driver(&rt9466_i2c_driver);
}
module_exit(rt9466_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("ShuFanLee <shufan_lee@richtek.com>");
MODULE_DESCRIPTION("RT9466 Charger Driver");
MODULE_VERSION(RT9466_DRV_VERSION);

/*
 * Release Note
 * 1.0.6
 * (1) Modify IBAT/IBUS ADC's coefficient
 * (2) Use gpio_request_one & gpio_to_irq instead of pinctrl
 *
 * 1.0.5
 * (1) Disable all irq in irq_init
 * (2) Modify rt9466_is_hw_exist, check vendor id and revision id separately
 *
 * 1.0.4
 * (1) Not to unmask the plug-in/out related IRQs in irq_init
 * PWR_RDYM/CHG_MIVRM/CHG_AICRM
 * VBUSOVM
 * TS_BAT_HOTM/TS_BAT_WARMM/TS_BAT_COOLM/TS_BAT_COLDM
 * CHG_OTPM/CHG_RVPM/CHG_ADPBADM/CHG_STATCM/CHG_FAULTM/TS_STATCM
 * IEOCM/TERMM/SSFINISHM/SSFINISHM/AICLMeasM
 * BST_BATUVM/PUMPX_DONEM/ADC_DONEM
 *
 * 1.0.3
 * (1) Copy default dts value before parsing dts
 * (2) Release rt_charger_sw_reset interface
 * (3) Add chip revision E4 (0x84)
 */
