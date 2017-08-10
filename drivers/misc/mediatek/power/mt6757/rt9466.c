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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <mt-plat/charging.h>
#include "rt9466.h"
#include "mtk_charger_intf.h"

#ifdef CONFIG_RT_REGMAP
#include <mt-plat/rt-regmap.h>
#endif

kal_bool chargin_hw_init_done = true; /* Used by MTK battery driver */

/* =============== */
/* RT9466 Variable */
/* =============== */

#define RT9466_DBG_MSG_I2C 1
#define RT9466_DBG_MSG_OTH 1

#define RT9466_ICHG_NUM 64
#define RT9466_ICHG_MIN 100  /* mA */
#define RT9466_ICHG_MAX 5000 /* mA */
#define RT9466_ICHG_STEP 100 /* mA */

#define RT9466_IEOC_NUM 16
#define RT9466_IEOC_MIN 100 /* mA */
#define RT9466_IEOC_MAX 850 /* mA */
#define RT9466_IEOC_STEP 50 /* mA */

#define RT9466_MIVR_NUM 128
#define RT9466_MIVR_MIN 3900   /* mV */
#define RT9466_MIVR_MAX 13400  /* mV */
#define RT9466_MIVR_STEP 100   /* mV */

#define RT9466_AICR_NUM 64
#define RT9466_AICR_MIN 100  /* mA */
#define RT9466_AICR_MAX 3250 /* mA */
#define RT9466_AICR_STEP 50  /* mA */

#define RT9466_BAT_VOREG_NUM 128
#define RT9466_BAT_VOREG_MIN 3900 /* mV */
#define RT9466_BAT_VOREG_MAX 4710 /* mV */
#define RT9466_BAT_VOREG_STEP 10  /* mV */

#define RT9466_BOOST_VOREG_NUM 64
#define RT9466_BOOST_VOREG_MIN 4425 /* mV */
#define RT9466_BOOST_VOREG_MAX 5825 /* mV */
#define RT9466_BOOST_VOREG_STEP 25  /* mV */

#define RT9466_VPREC_NUM 16
#define RT9466_VPREC_MIN 2000 /* mV */
#define RT9466_VPREC_MAX 3500 /* mV */
#define RT9466_VPREC_STEP 100 /* mV */

#define RT9466_IPREC_NUM 16
#define RT9466_IPREC_MIN 100 /* mA */
#define RT9466_IPREC_MAX 850 /* mA */
#define RT9466_IPREC_STEP 50 /* mA */

/* Watchdog fast-charge timer */
#define RT9466_WT_FC_NUM 8
#define RT9466_WT_FC_MIN 4  /* hour */
#define RT9466_WT_FC_MAX 20 /* hour */
#define RT9466_WT_FC_STEP 2 /* hour */

/* IR compensation */
#define RT9466_IRCMP_RES_NUM 8
#define RT9466_IRCMP_RES_MIN 0   /* ohm */
#define RT9466_IRCMP_RES_MAX 140 /* ohm */
#define RT9466_IRCMP_RES_STEP 20 /* ohm */

/* IR compensation maximum voltage clamp */
#define RT9466_IRCMP_VCLAMP_NUM 8
#define RT9466_IRCMP_VCLAMP_MIN 0   /* mV */
#define RT9466_IRCMP_VCLAMP_MAX 224 /* mV */
#define RT9466_IRCMP_VCLAMP_STEP 32 /* mV */

#define RT9466_BOOST_OC_THRESHOLD_NUM 8
static const u32 rt9466_boost_oc_threshold[RT9466_BOOST_OC_THRESHOLD_NUM] = {
	500, 700, 1100, 1300, 1800, 2100, 2400, 3000,
}; /* mA */


/* Charging status name */
static const char *rt9466_chg_status_name[RT_CHG_STATUS_MAX] = {
	"ready", "progress", "done", "fault",
};

/* DTS configuration */
enum {
	RT9466_DTS_CFG_ICHG = 0,
	RT9466_DTS_CFG_AICR,
	RT9466_DTS_CFG_MIVR,
	RT9466_DTS_CFG_IEOC,
	RT9466_DTS_CFG_TE,
	RT9466_DTS_CFG_MAX,
};
static const char *rt9466_dts_cfg_name[RT9466_DTS_CFG_MAX] = {
	"ichg", "aicr", "mivr", "ieoc", "te",
};

/* ======================= */
/* Address & Default value */
/* ======================= */


static const unsigned char rt9466_reg_addr[RT9466_REG_IDX_MAX] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x19, 0x1A,
	0x40, 0x42, 0x43, 0x44, 0x45,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65,
};

static const unsigned char rt9466_reg_def_val[RT9466_REG_IDX_MAX] = {
	0x00, 0x10, 0x0B, 0x23, 0x3C, 0x67, 0x0B, 0x4C,
	0xA3, 0x3C, 0x58, 0x2C, 0x00, 0x52, 0x05, 0x00,
	0x10, 0x00, 0x00, 0x40,
	0x81, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

#define RT9466_REG_EN_HIDDEN_MODE_MAX 8
static const unsigned char rt9466_reg_en_hidden_mode[RT9466_REG_EN_HIDDEN_MODE_MAX] = {
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
};

static const unsigned char rt9466_val_en_hidden_mode[RT9466_REG_EN_HIDDEN_MODE_MAX] = {
	0x49, 0x32, 0xB6, 0x27, 0x48, 0x18, 0x03, 0xE2,
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
 */
static const int rt9466_adc_unit[RT9466_ADC_MAX] = {
	0, 25, 10, 5, 5, 0, 0, 0, 50, 50, 0, 5, 2,
};

static const int rt9466_adc_offset[RT9466_ADC_MAX] = {
	0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -40, 0,
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

static rt_register_map_t rt9466_regmap_map[RT9466_REG_IDX_MAX] = {
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

static struct rt_regmap_properties rt9466_regmap_prop = {
	.name = "rt9466",
	.aliases = "rt9466",
	.register_num = RT9466_REG_IDX_MAX,
	.rm = rt9466_regmap_map,
	.rt_regmap_mode = RT_SINGLE_BYTE | RT_CACHE_DISABLE | RT_IO_PASS_THROUGH,
	.io_log_en = 1,
};
#endif /* CONFIG_RT_REGMAP */


struct rt9466_info {
	struct i2c_client *i2c;
	int dts_cfg[RT9466_DTS_CFG_MAX];
#ifdef CONFIG_RT_REGMAP
	struct rt_regmap_device *regmap_dev;
#endif
};

static struct rt9466_info g_rt9466_info = {
	.i2c = NULL,
	.dts_cfg = {2000, 500, 4400, 250, 1},
#ifdef CONFIG_RT_REGMAP
	.regmap_dev = NULL,
#endif
};

/* ========================= */
/* I2C operations            */
/* ========================= */

#ifdef CONFIG_RT_REGMAP
static int rt9466_regmap_read(void *client, u32 addr, int leng, void *dst)
{
	int ret = 0;
	struct i2c_client *i2c = NULL;

	i2c = (struct i2c_client *)client;
	ret = i2c_smbus_read_i2c_block_data(i2c, addr, leng, dst);

#if RT9466_DBG_MSG_I2C
	if (ret < 0)
		pr_info("%s: I2CR[0x%02X] failed\n", __func__, addr);
	else
		pr_info("%s: I2CR[0x%02X] = 0x%02X\n", __func__, addr, *((u8 *)dst));
#endif

	return ret;
}

static int rt9466_regmap_write(void *client, u32 addr, int leng, const void *src)
{
	int ret = 0;
	struct i2c_client *i2c = NULL;

	i2c = (struct i2c_client *)client;
	ret = i2c_smbus_write_i2c_block_data(i2c, addr, leng, src);

#if RT9466_DBG_MSG_I2C
	if (ret < 0)
		pr_info("%s: I2CW[0x%02X] = 0x%02X failed\n", __func__, addr,
			*((u8 *)src));
	else
		pr_info("%s: I2CW[0x%02X] = 0x%02X\n", __func__, addr, *((u8 *)src));
#endif

	return ret;
}

static struct rt_regmap_fops rt9466_regmap_fops = {
	.read_device = rt9466_regmap_read,
	.write_device = rt9466_regmap_write,
};


static int rt9466_register_rt_regmap(void)
{
	int ret = 0;

	pr_info("%s: starts\n", __func__);
	g_rt9466_info.regmap_dev = rt_regmap_device_register(
		&rt9466_regmap_prop,
		&rt9466_regmap_fops,
		&(g_rt9466_info.i2c->dev),
		g_rt9466_info.i2c,
		&g_rt9466_info
	);

	if (!g_rt9466_info.regmap_dev) {
		dev_err(&g_rt9466_info.i2c->dev, "register regmap device failed\n");
		return -EINVAL;
	}

	pr_info("%s: ends\n", __func__);
	return ret;
}

#else /* Not defined CONFIG_RT_REGMAP */

static int rt9466_write_device(struct i2c_client *i2c, u32 reg, int leng,
	const void *src)
{
	int ret = 0;

	if (leng > 1) {
		ret = i2c_smbus_write_i2c_block_data(i2c, reg, leng, src);
	} else {
		ret = i2c_smbus_write_byte_data(i2c, reg, *((u8 *)src));
		if (ret < 0)
			return ret;
	}

#if RT9466_DBG_MSG_I2C
	if (ret < 0)
		pr_info("%s: I2CW[0x%02X] = 0x%02X failed\n", __func__, reg,
			*((u8 *)src));
	else
		pr_info("%s: I2CW[0x%02X] = 0x%02X\n", __func__, reg, *((u8 *)src));
#endif

	return ret;
}

static int rt9466_read_device(struct i2c_client *i2c, u32 reg, int leng,
	void *dst)
{
	int ret = 0;

	if (leng > 1) {
		ret = i2c_smbus_read_i2c_block_data(i2c, reg, leng, dst);
	} else {
		ret = i2c_smbus_read_byte_data(i2c, reg);
		if (ret < 0)
			return ret;

		*(u8 *)dst = (u8)ret;
	}

#if RT9466_DBG_MSG_I2C
	if (ret < 0)
		pr_info("%s: I2CR[0x%02X] failed\n", __func__, reg);
	else
		pr_info("%s: I2CR[0x%02X] = 0x%02X\n", __func__, reg, *((u8 *)dst));
#endif

	return ret;
}
#endif /* CONFIG_RT_REGMAP */

static int rt9466_i2c_write_byte(u8 cmd, u8 data)
{
	int ret = 0;

#ifdef CONFIG_RT_REGMAP
	ret = rt9466_regmap_write(g_rt9466_info.i2c, cmd, 1, &data);
#else
	ret = rt9466_write_device(g_rt9466_info.i2c, cmd, 1, &data);
#endif

	return ret;
}


static int rt9466_i2c_read_byte(u8 cmd)
{
	int ret = 0, ret_val = 0;

#ifdef CONFIG_RT_REGMAP
	ret = rt9466_regmap_read(g_rt9466_info.i2c, cmd, 1, &ret_val);
#else
	ret = rt9466_read_device(g_rt9466_info.i2c, cmd, 1, &ret_val);
#endif

	if (ret < 0)
		return ret;

	ret_val = ret_val & 0xFF;

	return ret_val;
}

static int rt9466_i2c_test_bit(u8 cmd, u8 shift)
{
	int ret = 0;

	ret = rt9466_i2c_read_byte(cmd);
	if (ret < 0)
		return ret;

	ret = ret & (1 << shift);

	return ret;
}

static int rt9466_i2c_update_bits(u8 cmd, u8 data, u8 mask)
{
	int ret = 0;
	u8 reg_data = 0;

	ret = rt9466_i2c_read_byte(cmd);
	if (ret < 0)
		return ret;

	reg_data = ret & 0xFF;
	reg_data &= ~mask;
	reg_data |= (data & mask);

	return rt9466_i2c_write_byte(cmd, reg_data);
}

#define rt9466_set_bit(reg, mask) \
	rt9466_i2c_update_bits(reg, mask, mask)

#define rt9466_clr_bit(reg, mask) \
	rt9466_i2c_update_bits(reg, 0x00, mask)

/* ================== */
/* Internal Functions */
/* ================== */

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
		if (target_value >= value_table[i] && target_value < value_table[i + 1])
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

static bool rt9466_is_hw_exist(void)
{
	int ret = 0;

	ret = rt9466_i2c_read_byte(RT9466_REG_DEVICE_ID);
	if (ret < 0)
		return false;

	if ((ret & 0xFF) != rt9466_reg_def_val[RT9466_REG_IDX_DEVICE_ID])
		return false;

	return true;
}

static int rt9466_set_fast_charge_timer(u32 hour)
{
	int ret = 0;
	u8 reg_fct = 0;

#if RT9466_DBG_MSG_OTH
	pr_info("%s: set fast charge timer to %d\n", __func__, hour);
#endif

	reg_fct = rt9466_find_closest_reg_value(RT9466_WT_FC_MIN, RT9466_WT_FC_MAX,
		RT9466_WT_FC_STEP, RT9466_WT_FC_NUM, hour);

	ret = rt9466_i2c_update_bits(
		RT9466_REG_CHG_CTRL12,
		reg_fct << RT9466_SHIFT_WT_FC,
		RT9466_MASK_WT_FC
	);

	return ret;
}

#ifdef CONFIG_MTK_BIF_SUPPORT
static int rt9466_set_ircmp_resistor(u32 resistor)
{
	int ret = 0;
	u8 reg_resistor = 0;

#if RT9466_DBG_MSG_OTH
	pr_info("%s: set ir comp resistor = %d\n", __func__, resistor);
#endif

	reg_resistor = rt9466_find_closest_reg_value(RT9466_IRCMP_RES_MIN,
		RT9466_IRCMP_RES_MAX, RT9466_IRCMP_RES_STEP, RT9466_IRCMP_RES_NUM,
		resistor);

	ret = rt9466_i2c_update_bits(
		RT9466_REG_CHG_CTRL18,
		reg_resistor << RT9466_SHIFT_IRCMP_RES,
		RT9466_MASK_IRCMP_RES
	);

	return ret;
}

static int rt9466_set_ircmp_vclamp(u32 vclamp)
{
	int ret = 0;
	u8 reg_vclamp = 0;

#if RT9466_DBG_MSG_OTH
	pr_info("%s: set ir comp vclamp = %d\n", __func__, vclamp);
#endif

	reg_vclamp = rt9466_find_closest_reg_value(RT9466_IRCMP_VCLAMP_MIN,
		RT9466_IRCMP_VCLAMP_MAX, RT9466_IRCMP_VCLAMP_STEP,
		RT9466_IRCMP_VCLAMP_NUM, vclamp);

	ret = rt9466_i2c_update_bits(
		RT9466_REG_CHG_CTRL18,
		reg_vclamp << RT9466_SHIFT_IRCMP_VCLAMP,
		RT9466_MASK_IRCMP_VCLAMP
	);

	return ret;
}
#endif /* CONFIG_MTK_BIF_SUPPORT */

static int rt9466_enable_ircmp(u8 enable)
{
	int ret = 0;

#if RT9466_DBG_MSG_OTH
	pr_info("%s: enable ircmp = %d\n", __func__, enable);
#endif

	ret = (enable ? rt9466_set_bit(RT9466_REG_CHG_CTRL18, RT9466_MASK_IRCMP_EN)
				: rt9466_clr_bit(RT9466_REG_CHG_CTRL18, RT9466_MASK_IRCMP_EN));

	return ret;
}

/* Hardware pin current limit */
static int rt9466_enable_ilim(u8 enable)
{
	int ret = 0;

#if RT9466_DBG_MSG_OTH
	pr_info("%s: enable ilim = %d\n", __func__, enable);
#endif

	ret = (enable ? rt9466_set_bit(RT9466_REG_CHG_CTRL3, RT9466_MASK_ILIM_EN)
		: rt9466_clr_bit(RT9466_REG_CHG_CTRL3, RT9466_MASK_ILIM_EN));

	return ret;
}

/* Enter hidden mode */
static int rt9466_enter_hidden_mode(void)
{
	int ret = 0, i = 0;

#if RT9466_DBG_MSG_OTH
	pr_info("%s: enter hidden mode\n", __func__);
#endif

	for (i = 0; i < RT9466_REG_EN_HIDDEN_MODE_MAX; i++) {
		ret = rt9466_i2c_write_byte(
			rt9466_reg_en_hidden_mode[i],
			rt9466_val_en_hidden_mode[i]
		);
		if (ret < 0) {
			pr_info("%s: enter hidden mode failed\n", __func__);
			return ret;
		}
	}

	return ret;
}

/* Software workaround */
static int rt9466_sw_workaround(void)
{
	int ret = 0;

#if RT9466_DBG_MSG_OTH
	pr_info("%s: starts\n", __func__);
#endif

	/* Enter hidden mode */
	ret = rt9466_enter_hidden_mode();
	if (ret < 0)
		return ret;

	/* ICC: modify sensing node, make it more accurate */
	ret = rt9466_i2c_write_byte(0x27, 0x00);
	if (ret < 0)
		return ret;

	/* DIMIN level */
	ret = rt9466_i2c_write_byte(0x28, 0x86);

	return ret;
}

static int rt9466_get_adc(enum rt9466_adc_sel adc_sel, int *adc_val)
{
	int ret = 0, i = 0;
	const int wait_retry = 5;
	u8 adc_h = 0, adc_l = 0;

	/* Select ADC to temperature channel*/
	ret = rt9466_i2c_update_bits(
		RT9466_REG_CHG_ADC,
		adc_sel << RT9466_SHIFT_ADC_IN_SEL,
		RT9466_MASK_ADC_IN_SEL
	);

	if (ret < 0) {
		pr_info("%s: select ADC to %d failed\n", __func__, adc_sel);
		return ret;
	}

	/* Start ADC conversation */
	ret = rt9466_set_bit(RT9466_REG_CHG_ADC, RT9466_MASK_ADC_START);
	if (ret < 0) {
		pr_info("%s: start ADC conversation failed\n", __func__);
		return ret;
	}

	/* Wait for ADC conversation */
	for (i = 0; i < wait_retry; i++) {
		mdelay(25);
		if (rt9466_i2c_test_bit(RT9466_REG_CHG_IRQ3, RT9466_SHIFT_ADC_DONEI) > 0) {
			pr_info("%s: sel = %d, wait_times = %d\n", __func__, adc_sel, i);
			break;
		}
		if ((i + 1) == wait_retry) {
			pr_info("%s: Wait ADC conversation failed\n", __func__);
			return -EINVAL;
		}
	}

	/* Read ADC data high byte */
	ret = rt9466_i2c_read_byte(RT9466_REG_ADC_DATA_H);
	if (ret < 0) {
		pr_info("%s: read ADC high byte data failed\n", __func__);
		return ret;
	}
	adc_h = ret & 0xFF;

	/* Read ADC data low byte */
	ret = rt9466_i2c_read_byte(RT9466_REG_ADC_DATA_L);
	if (ret < 0) {
		pr_info("%s: read ADC low byte data failed\n", __func__);
		return ret;
	}
	adc_l = ret & 0xFF;
	pr_info("%s: adc_sel = %d, adc_h = 0x%02X, adc_l = 0x%02X\n",
		__func__, adc_sel, adc_h, adc_l);

	/* Calculate ADC value */
	*adc_val = ((adc_h * 256 + adc_l) * rt9466_adc_unit[adc_sel])
		+ rt9466_adc_offset[adc_sel];

	return ret;
}

/* =========================================================== */
/* The following is implementation for interface of rt_charger */
/* =========================================================== */

/* Set register's value to default */
int rt_charger_hw_init(void)
{
	int ret = 0;
	int i = 0;

#if RT9466_DBG_MSG_OTH
	pr_info("%s: starts\n", __func__);
#endif

	for (i = 0; i < RT9466_REG_IDX_MAX; i++) {
		ret = rt9466_i2c_write_byte(
			rt9466_reg_addr[i],
			rt9466_reg_def_val[i]
		);

		if (ret < 0)
			return ret;
	}

	return ret;
}

int rt_charger_sw_init(void)
{
	int ret = 0;

#if RT9466_DBG_MSG_OTH
	pr_info("%s: starts\n", __func__);
#endif

	/* Disable hardware ILIM */
	ret = rt9466_enable_ilim(0);
	if (ret < 0)
		pr_info("%s: Disable ilim failed\n", __func__);

	ret = rt_charger_set_ichg(g_rt9466_info.dts_cfg[RT9466_DTS_CFG_ICHG]);
	if (ret < 0)
		pr_info("%s: Set ichg failed\n", __func__);

	ret = rt_charger_set_aicr(g_rt9466_info.dts_cfg[RT9466_DTS_CFG_AICR]);
	if (ret < 0)
		pr_info("%s: Set aicr failed\n", __func__);

	ret = rt_charger_set_mivr(g_rt9466_info.dts_cfg[RT9466_DTS_CFG_MIVR]);
	if (ret < 0)
		pr_info("%s: Set mivr failed\n", __func__);

	ret = rt_charger_set_ieoc(g_rt9466_info.dts_cfg[RT9466_DTS_CFG_IEOC]);
	if (ret < 0)
		pr_info("%s: Set ieoc failed\n", __func__);

	ret = rt_charger_enable_te(g_rt9466_info.dts_cfg[RT9466_DTS_CFG_TE]);
	if (ret < 0)
		pr_info("%s: Set te failed\n", __func__);

	/* Set fast charge timer to 12 hours */
	ret = rt9466_set_fast_charge_timer(12);
	if (ret < 0)
		pr_info("%s: Set fast timer failed\n", __func__);

	/* Set ircomp according to BIF */
#ifdef CONFIG_MTK_BIF_SUPPORT
	ret = rt9466_set_ircmp_resistor(80);
	if (ret < 0)
		pr_info("%s: Set IR compensation resistor failed\n", __func__);

	ret = rt9466_set_ircmp_vclamp(224);
	if (ret < 0)
		pr_info("%s: Set IR compensation voltage clamp failed\n", __func__);

	ret = rt9466_enable_ircmp(1);
	if (ret < 0)
		pr_info("%s: Eanble IR compensation failed\n", __func__);
#else
	ret = rt9466_enable_ircmp(0);
	if (ret < 0)
		pr_info("%s: Disable IR compensation failed\n", __func__);
#endif
	return ret;
}

int rt_charger_dump_register(void)
{
	int i = 0, ret = 0;
	u32 ichg = 0, aicr = 0, mivr = 0, ieoc = 0;
	u8 chg_enable = 0;
	int adc_vsys = 0, adc_vbat = 0, adc_ibus = 0, adc_ibat = 0;
	enum rt_charging_status chg_status = RT_CHG_STATUS_READY;

	for (i = 0; i < RT9466_REG_IDX_MAX; i++) {
		ret = rt9466_i2c_read_byte(rt9466_reg_addr[i]);
		if (ret < 0)
			return ret;
	}

	ret = rt_charger_get_ichg(&ichg);
	ret = rt_charger_get_mivr(&mivr);
	ret = rt_charger_get_aicr(&aicr);
	ret = rt_charger_get_ieoc(&ieoc);
	ret = rt_charger_is_charging_enable(&chg_enable);
	ret = rt_charger_get_charging_status(&chg_status);
	ret = rt9466_get_adc(RT9466_ADC_VSYS, &adc_vsys);
	ret = rt9466_get_adc(RT9466_ADC_VBAT, &adc_vbat);
	ret = rt9466_get_adc(RT9466_ADC_IBUS, &adc_ibus);
	ret = rt9466_get_adc(RT9466_ADC_IBAT, &adc_ibat);

	pr_info("%s: ICHG = %dmA, AICR = %dmA, MIVR = %dmV, IEOC = %dmA, CHG_EN = %d, CHG_STATUS = %s\n",
		__func__, ichg, aicr, mivr, ieoc, chg_enable,
		rt9466_chg_status_name[chg_status]);

	pr_info("%s: VSYS = %dmV, VBAT = %dmV, IBUS = %dmA, IBAT = %dmA\n",
		__func__, adc_vsys, adc_vbat, adc_ibus, adc_ibat);

	return ret;
}

int rt_charger_enable_charging(const u8 enable)
{
	int ret = 0;

	ret = (enable ? rt9466_set_bit(RT9466_REG_CHG_CTRL2, RT9466_MASK_CHG_EN)
		: rt9466_clr_bit(RT9466_REG_CHG_CTRL2, RT9466_MASK_CHG_EN));

	return ret;
}

int rt_charger_enable_hz(u8 enable)
{
	int ret = 0;

	ret = (enable ? rt9466_set_bit(RT9466_REG_CHG_CTRL1, RT9466_MASK_HZ_EN)
		: rt9466_clr_bit(RT9466_REG_CHG_CTRL1, RT9466_MASK_CHG_EN));

	return ret;
}

int rt_charger_enable_te(const u8 enable)
{
	int ret = 0;

	ret = (enable ? rt9466_set_bit(RT9466_REG_CHG_CTRL2, RT9466_MASK_TE_EN)
		: rt9466_clr_bit(RT9466_REG_CHG_CTRL2, RT9466_MASK_TE_EN));

	return ret;
}

int rt_charger_enable_te_shutdown(const u8 enable)
{
	return -ENOTSUPP;
}

int rt_charger_enable_timer(const u8 enable)
{
	int ret = 0;

	ret = (enable ? rt9466_set_bit(RT9466_REG_CHG_CTRL12, RT9466_MASK_TMR_EN)
		: rt9466_clr_bit(RT9466_REG_CHG_CTRL12, RT9466_MASK_TMR_EN));

	return ret;
}

int rt_charger_enable_otg(const u8 enable)
{
	int ret = 0, i = 0;
	const int ss_wait_times = 5; /* soft start wait times */

	/* Set OTG_OC to 500mA */
	ret = rt_charger_set_boost_current_limit(500);
	if (ret < 0)
		return ret;

	/* Switch OPA mode to boost mode */
#if RT9466_DBG_MSG_OTH
	pr_info("%s: otg enable = %d\n", __func__, enable);
#endif
	ret = (enable ? rt9466_set_bit(RT9466_REG_CHG_CTRL1, RT9466_MASK_OPA_MODE)
		: rt9466_clr_bit(RT9466_REG_CHG_CTRL1, RT9466_MASK_OPA_MODE));

	if (!enable) {
		/* Workaround: reg[0x25] = 0x0F after leaving OTG mode */
		ret = rt9466_i2c_write_byte(0x25, 0x0F);
		return ret;
	}

	/* Wait for soft start finish interrupt */
	for (i = 0; i < ss_wait_times; i++) {
		ret = rt9466_i2c_test_bit(RT9466_REG_CHG_IRQ2, RT9466_SHIFT_SSFINISHI);
		if (ret > 0) {
			/* Woraround reg[0x25] = 0x00 after entering OTG mode */
			ret = rt9466_i2c_write_byte(0x25, 0x00);
			pr_info("%s: otg soft start successfully\n", __func__);
			break;
		}
		mdelay(100);
		if ((i + 1) == ss_wait_times)
			pr_info("%s: otg soft start failed\n", __func__);
	}

	return ret;
}

int rt_charger_enable_pumpX(const u8 enable)
{
	int ret = 0, i = 0, retry_times = 5;

#if RT9466_DBG_MSG_OTH
	pr_info("%s: enable pumpX = %d\n", __func__, enable);
#endif

	ret = rt_charger_set_aicr(800);
	if (ret < 0)
		return ret;

	ret = rt_charger_set_ichg(2050);
	if (ret < 0)
		return ret;

	ret = (enable ? rt9466_set_bit(RT9466_REG_CHG_CTRL17, RT9466_MASK_PUMPX_EN)
		: rt9466_clr_bit(RT9466_REG_CHG_CTRL17, RT9466_MASK_PUMPX_EN));

	if (ret < 0)
		return ret;

	for (i = 0; i < retry_times; i++) {
		if (rt9466_i2c_test_bit(RT9466_REG_CHG_CTRL17, RT9466_SHIFT_PUMPX_EN) == 0)
			break;
		msleep(2500);
		if ((i + 1) == retry_times) {
			pr_info("%s: pumpX wait failed\n", __func__);
			return -EINVAL;
		}
	}

	return ret;
}

int rt_charger_enable_aicr(const u8 enable)
{
	int ret = 0;

	ret = (enable ? rt9466_set_bit(RT9466_REG_CHG_CTRL3, RT9466_MASK_AICR_EN)
		: rt9466_clr_bit(RT9466_REG_CHG_CTRL3, RT9466_MASK_AICR_EN));

	return ret;
}

int rt_charger_enable_mivr(const u8 enable)
{
	int ret = 0;

	ret = (enable ? rt9466_set_bit(RT9466_REG_CHG_CTRL6, RT9466_MASK_MIVR_EN)
		: rt9466_clr_bit(RT9466_REG_CHG_CTRL6, RT9466_MASK_MIVR_EN));

	return ret;
}

int rt_charger_enable_power_path(const u8 enable)
{
	int ret = 0;

	ret = (enable ? rt9466_set_bit(RT9466_REG_CHG_CTRL2, RT9466_MASK_CFO_EN)
		: rt9466_clr_bit(RT9466_REG_CHG_CTRL2, RT9466_MASK_CFO_EN));

	return ret;
}

int rt_charger_set_ichg(const u32 ichg)
{
	int ret = 0;

	/* Find corresponding reg value */
	u8 reg_ichg = rt9466_find_closest_reg_value(RT9466_ICHG_MIN,
		RT9466_ICHG_MAX, RT9466_ICHG_STEP, RT9466_ICHG_NUM, ichg);

#if RT9466_DBG_MSG_OTH
	pr_info("%s: ichg = %d\n", __func__, ichg);
#endif

	ret = rt9466_i2c_update_bits(
		RT9466_REG_CHG_CTRL7,
		reg_ichg << RT9466_SHIFT_ICHG,
		RT9466_MASK_ICHG
	);

	return ret;
}

int rt_charger_set_ieoc(const u32 ieoc)
{
	int ret = 0;

	/* Find corresponding reg value */
	u8 reg_ieoc = rt9466_find_closest_reg_value(RT9466_IEOC_MIN,
		RT9466_IEOC_MAX, RT9466_IEOC_STEP, RT9466_IEOC_NUM, ieoc);

#if RT9466_DBG_MSG_OTH
	pr_info("%s: ieoc = %d\n", __func__, ieoc);
#endif

	ret = rt9466_i2c_update_bits(
		RT9466_REG_CHG_CTRL9,
		reg_ieoc << RT9466_SHIFT_IEOC,
		RT9466_MASK_IEOC
	);

	return ret;
}


int rt_charger_set_aicr(const u32 aicr)
{
	int ret = 0;

	/* Find corresponding reg value */
	u8 reg_aicr = rt9466_find_closest_reg_value(RT9466_AICR_MIN,
		RT9466_AICR_MAX, RT9466_AICR_STEP, RT9466_AICR_NUM, aicr);

#if RT9466_DBG_MSG_OTH
	pr_info("%s: aicr = %d\n", __func__, aicr);
#endif

	ret = rt9466_i2c_update_bits(
		RT9466_REG_CHG_CTRL3,
		reg_aicr << RT9466_SHIFT_AICR,
		RT9466_MASK_AICR
	);

	return ret;
}


int rt_charger_set_mivr(const u32 mivr)
{
	int ret = 0;

	/* Find corresponding reg value */
	u8 reg_mivr = rt9466_find_closest_reg_value(RT9466_MIVR_MIN,
		RT9466_MIVR_MAX, RT9466_MIVR_STEP, RT9466_MIVR_NUM, mivr);

#if RT9466_DBG_MSG_OTH
	pr_info("%s: mivr = %d\n", __func__, mivr);
#endif

	ret = rt9466_i2c_update_bits(
		RT9466_REG_CHG_CTRL6,
		reg_mivr << RT9466_SHIFT_MIVR,
		RT9466_MASK_MIVR
	);

	return ret;
}

int rt_charger_set_battery_voreg(const u32 voreg)
{
	int ret = 0;

	u8 reg_voreg = rt9466_find_closest_reg_value(RT9466_BAT_VOREG_MIN,
		RT9466_BAT_VOREG_MAX, RT9466_BAT_VOREG_STEP, RT9466_BAT_VOREG_NUM, voreg);

#if RT9466_DBG_MSG_OTH
	pr_info("%s: bat voreg = %d\n", __func__, voreg);
#endif

	ret = rt9466_i2c_update_bits(
		RT9466_REG_CHG_CTRL4,
		reg_voreg << RT9466_SHIFT_BAT_VOREG,
		RT9466_MASK_BAT_VOREG
	);

	return ret;
}

int rt_charger_set_boost_voreg(const u32 voreg)
{
	int ret = 0;

	u8 reg_voreg = rt9466_find_closest_reg_value(RT9466_BOOST_VOREG_MIN,
		RT9466_BOOST_VOREG_MAX, RT9466_BOOST_VOREG_STEP,
		RT9466_BOOST_VOREG_NUM, voreg);

#if RT9466_DBG_MSG_OTH
	pr_info("%s: boost voreg = %d\n", __func__, voreg);
#endif

	ret = rt9466_i2c_update_bits(
		RT9466_REG_CHG_CTRL5,
		reg_voreg << RT9466_SHIFT_BOOST_VOREG,
		RT9466_MASK_BOOST_VOREG
	);

	return ret;
}

int rt_charger_set_boost_current_limit(const u32 ilimit)
{
	int ret = 0;
	u8 reg_ilimit = 0;

	reg_ilimit = rt9466_find_closest_reg_value_via_table(
		rt9466_boost_oc_threshold,
		RT9466_BOOST_OC_THRESHOLD_NUM,
		ilimit
	);

#if RT9466_DBG_MSG_OTH
	pr_info("%s: boost ilimit = %d\n", __func__, ilimit);
#endif

	ret = rt9466_i2c_update_bits(
		RT9466_REG_CHG_CTRL10,
		reg_ilimit << RT9466_SHIFT_BOOST_OC,
		RT9466_MASK_BOOST_OC
	);

	return ret;
}

int rt_charger_set_ta_current_pattern(const u8 is_pump_up)
{
	int ret = 0;

#if RT9466_DBG_MSG_OTH
	pr_info("%s: pe1.0 pump_up = %d\n", __func__, is_pump_up);
#endif

	/* Set to PE1.0 */
	ret = rt9466_clr_bit(RT9466_REG_CHG_CTRL17, RT9466_MASK_PUMPX_20_10);

	/* Set Pump Up/Down */
	ret = (is_pump_up ?
		rt9466_set_bit(RT9466_REG_CHG_CTRL17, RT9466_MASK_PUMPX_UP_DN)
		: rt9466_clr_bit(RT9466_REG_CHG_CTRL17, RT9466_MASK_PUMPX_UP_DN));

	return ret;
}

int rt_charger_get_charging_status(enum rt_charging_status *chg_stat)
{
	int ret = 0;

	ret = rt9466_i2c_read_byte(RT9466_REG_CHG_STAT);
	if (ret < 0)
		return ret;

	*chg_stat = (ret & RT9466_MASK_CHG_STAT) >> RT9466_SHIFT_CHG_STAT;

	return ret;
}

int rt_charger_get_ichg(u32 *ichg)
{
	int ret = 0;
	u8 reg_ichg = 0;

	ret = rt9466_i2c_read_byte(RT9466_REG_CHG_CTRL7);
	if (ret < 0)
		return ret;

	reg_ichg = (ret & RT9466_MASK_ICHG) >> RT9466_SHIFT_ICHG;
	*ichg = rt9466_find_closest_real_value(RT9466_ICHG_MIN, RT9466_ICHG_MAX,
		RT9466_ICHG_STEP, reg_ichg);

	return ret;
}

int rt_charger_get_ieoc(u32 *ieoc)
{
	int ret = 0;
	u8 reg_ieoc = 0;

	ret = rt9466_i2c_read_byte(RT9466_REG_CHG_CTRL9);
	if (ret < 0)
		return ret;

	reg_ieoc = (ret & RT9466_MASK_IEOC) >> RT9466_SHIFT_IEOC;
	*ieoc = rt9466_find_closest_real_value(RT9466_IEOC_MIN, RT9466_IEOC_MAX,
		RT9466_IEOC_STEP, reg_ieoc);

	return ret;
}

int rt_charger_get_aicr(u32 *aicr)
{
	int ret = 0;
	u8 reg_aicr = 0;

	ret = rt9466_i2c_read_byte(RT9466_REG_CHG_CTRL3);
	if (ret < 0)
		return ret;

	reg_aicr = (ret & RT9466_MASK_AICR) >> RT9466_SHIFT_AICR;
	*aicr = rt9466_find_closest_real_value(RT9466_AICR_MIN, RT9466_AICR_MAX,
		RT9466_AICR_STEP, reg_aicr);

	return ret;
}

int rt_charger_get_mivr(u32 *mivr)
{
	int ret = 0;
	u8 reg_mivr = 0;

	ret = rt9466_i2c_read_byte(RT9466_REG_CHG_CTRL6);
	if (ret < 0)
		return ret;
	reg_mivr = ((ret & RT9466_MASK_MIVR) >> RT9466_SHIFT_MIVR) & 0xFF;

	*mivr = rt9466_find_closest_real_value(RT9466_MIVR_MIN, RT9466_MIVR_MAX,
		RT9466_MIVR_STEP, reg_mivr);


	return ret;
}

int rt_charger_get_battery_voreg(u32 *voreg)
{
	int ret = 0;
	u8 reg_voreg = 0;

	ret = rt9466_i2c_read_byte(RT9466_REG_CHG_CTRL4);
	if (ret < 0)
		return ret;

	reg_voreg = ((ret & RT9466_MASK_BAT_VOREG) >> RT9466_SHIFT_BAT_VOREG) & 0xFF;

	*voreg = rt9466_find_closest_real_value(RT9466_BAT_VOREG_MIN,
		RT9466_BAT_VOREG_MAX, RT9466_BAT_VOREG_STEP, reg_voreg);

	return ret;
}

int rt_charger_get_boost_voreg(u32 *voreg)
{
	int ret = 0;
	u8 reg_voreg = 0;

	ret = rt9466_i2c_read_byte(RT9466_REG_CHG_CTRL5);
	if (ret < 0)
		return ret;

	reg_voreg = ((ret & RT9466_MASK_BOOST_VOREG) >> RT9466_SHIFT_BOOST_VOREG) & 0xFF;

	*voreg = rt9466_find_closest_real_value(RT9466_BOOST_VOREG_MIN,
		RT9466_BOOST_VOREG_MAX, RT9466_BOOST_VOREG_STEP, reg_voreg);

	return ret;
}

int rt_charger_get_temperature(int *min_temp, int *max_temp)
{
	int ret = 0, adc_temp = 0;

	/* Get value from ADC */
	ret = rt9466_get_adc(RT9466_ADC_TEMP_JC, &adc_temp);
	if (ret < 0)
		return ret;

	*min_temp = adc_temp;
	*max_temp = adc_temp;

	pr_info("%s: temperature = %d\n", __func__, *min_temp);
	return ret;
}

int rt_charger_is_charging_enable(u8 *enable)
{
	int ret = 0;

	ret = rt9466_i2c_read_byte(RT9466_REG_CHG_CTRL2);
	if (ret < 0)
		return ret;

	*enable = ((ret & RT9466_MASK_CHG_EN) >> RT9466_SHIFT_CHG_EN) & 0xFF;

	return ret;
}

int rt_charger_reset_wchdog_timer(void)
{
	return -ENOTSUPP;
}


/* ========================= */
/* i2c driver function */
/* ========================= */

static void rt9466_parse_dts_cfg(void)
{
	int i = 0, ret = 0;

	for (i = 0; i < RT9466_DTS_CFG_MAX; i++) {
#ifdef CONFIG_OF
		ret = of_property_read_u32(
			g_rt9466_info.i2c->dev.of_node,
			rt9466_dts_cfg_name[i],
			&g_rt9466_info.dts_cfg[i]
		);
		if (ret < 0) {
			pr_info("%s: no property[%s], set default = %d\n",
				__func__, rt9466_dts_cfg_name[i], g_rt9466_info.dts_cfg[i]);
		}
#endif /* CONFIG_OF */

#if RT9466_DBG_MSG_OTH
	pr_info("%s: %s = %d\n", __func__, rt9466_dts_cfg_name[i],
		g_rt9466_info.dts_cfg[i]);
#endif
	}
}


static int rt9466_probe(struct i2c_client *i2c,
	const struct i2c_device_id *dev_id)
{
	int ret = 0;

	pr_info("%s: starts\n", __func__);
	i2c_set_clientdata(i2c, &g_rt9466_info);

	g_rt9466_info.i2c = i2c;

	/* Is HW exist */
	if (!rt9466_is_hw_exist()) {
		pr_info("%s: no rt9466 exists\n", __func__);
		BUG_ON(1);
		return -ENODEV;
	}

	/* Parse default configuration from dts */
	rt9466_parse_dts_cfg();

#ifdef CONFIG_RT_REGMAP
	ret = rt9466_register_rt_regmap();
	if (ret < 0)
		return ret;
#endif

	ret = rt_charger_hw_init();
	if (ret < 0) {
		pr_info("%s: set register to default value failed\n", __func__);
		BUG_ON(1);
		return -EINVAL;
	}

	ret = rt_charger_sw_init();
	if (ret < 0) {
		pr_info("%s: set failed\n", __func__);
		BUG_ON(1);
		return -EINVAL;
	}

	ret = rt9466_sw_workaround();
	if (ret < 0) {
		pr_info("%s: software workaround failed\n", __func__);
		BUG_ON(1);
		return -EINVAL;
	}

	rt_charger_dump_register();
	chargin_hw_init_done = true;
	pr_info("%s: ends\n", __func__);
	return ret;
}


static int rt9466_remove(struct i2c_client *client)
{
	int ret = 0;

	pr_info("%s: starts\n", __func__);

#ifdef CONFIG_RT_REGMAP
	rt_regmap_device_unregister(g_rt9466_info.regmap_dev);
#endif
	return ret;
}

static const struct i2c_device_id rt9466_i2c_id[] = {
	{"rt9466", 0},
	{}
};

#ifdef CONFIG_OF
static const struct of_device_id rt9466_of_match[] = {
	{ .compatible = "mediatek,switching_charger", },
	{},
};
#else

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
	.id_table = rt9466_i2c_id,
};


static int __init rt9466_init(void)
{
	int ret = 0;

#ifdef CONFIG_OF
	pr_info("%s: with dts\n", __func__);
#else
	pr_info("%s: without dts\n", __func__);
	i2c_register_board_info(RT9466_BUSNUM, &rt9466_i2c_board_info, 1);
#endif

	ret = i2c_add_driver(&rt9466_i2c_driver);
	if (ret < 0)
		pr_info("%s: register i2c driver failed\n", __func__);

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
MODULE_DESCRIPTION("RT9466 Charger Driver E2");
MODULE_VERSION("0.0.1_MTK");
