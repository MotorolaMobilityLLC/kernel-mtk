/*
 * Copyright (C) 2016 MediaTek Inc.
 * cy_huang <cy_huang@richtek.com>
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
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/delay.h>

#include <mtk_charger_intf.h>
#include <mt-plat/charging.h>
#include <mt-plat/battery_common.h>
#include <mach/mtk_pe.h>

#include "inc/rt5081_pmu_charger.h"
#include "inc/rt5081_pmu.h"


/* ======================= */
/* RT5081 Charger Variable */
/* ======================= */

struct rt5081_pmu_charger_desc {
	u32 ichg;
	u32 aicr;
	u32 mivr;
	u32 cv;
	u32 ieoc;
	u32 safety_timer;
	u32 ircmp_resistor;
	u32 ircmp_vclamp;
	u32 dc_wdt;
	bool enable_te;
	bool enable_wdt;
};

struct rt5081_pmu_charger_data {
	struct mtk_charger_info mchr_info;
	struct mutex adc_access_lock;
	struct mutex irq_access_lock;
	struct rt5081_pmu_chip *chip;
	struct device *dev;
	struct rt5081_pmu_charger_desc *chg_desc;
	struct work_struct discharging_work;
	struct workqueue_struct *discharging_workqueue;
	bool err_state;
	int i2c_log_level;
	u8 chg_stat1;
	u8 chg_stat5;
	u8 chg_stat6;
};

/* These default values will be used if there's no property in dts */
static struct rt5081_pmu_charger_desc rt5081_default_chg_desc = {
	.ichg = 2000,
	.aicr = 500,
	.mivr = 4400,
	.cv = 4350,
	.ieoc = 250,
	.safety_timer = 12,
	.ircmp_resistor = 80,
	.ircmp_vclamp = 224,
	.dc_wdt = 4000,
	.enable_te = true,
	.enable_wdt = true,
};


static const u32 rt5081_otg_oc_threshold[] = {
	500, 700, 1100, 1300, 1800, 2100, 2400, 3000,
}; /* mA */

static const u32 rt5081_dc_vbatov_lvl[] = {
	104, 108, 119,
}; /* % * VOREG */

static const u32 rt5081_dc_watchdog_timer[] = {
	125, 250, 500, 1000, 2000, 4000, 8000,
}; /* ms */

enum rt5081_charging_status {
	RT5081_CHG_STATUS_READY = 0,
	RT5081_CHG_STATUS_PROGRESS,
	RT5081_CHG_STATUS_DONE,
	RT5081_CHG_STATUS_FAULT,
	RT5081_CHG_STATUS_MAX,
};

/* Charging status name */
static const char *rt5081_chg_status_name[RT5081_CHG_STATUS_MAX] = {
	"ready", "progress", "done", "fault",
};

#if 0 /* TBD, Ask Thor */
#define rt5081_REG_EN_HIDDEN_MODE_MAX 8
static const unsigned char rt5081_reg_en_hidden_mode[rt5081_REG_EN_HIDDEN_MODE_MAX] = {
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
};

static const unsigned char rt5081_val_en_hidden_mode[rt5081_REG_EN_HIDDEN_MODE_MAX] = {
	0x49, 0x32, 0xB6, 0x27, 0x48, 0x18, 0x03, 0xE2,
};
#endif

enum rt5081_iin_limit_sel {
	RT5081_IIMLMTSEL_AICR_3250 = 0,
	RT5081_IIMLMTSEL_CHR_TYPE,
	RT5081_IINLMTSEL_AICR,
	RT5081_IINLMTSEL_LOWER_LEVEL, /* lower of above two */
};

enum rt5081_adc_sel {
	RT5081_ADC_VBUS_DIV5 = 1,
	RT5081_ADC_VBUS_DIV2,
	RT5081_ADC_VSYS,
	RT5081_ADC_VBAT,
	RT5081_ADC_TS_BAT = 6,
	RT5081_ADC_IBUS = 8,
	RT5081_ADC_IBAT,
	RT5081_ADC_CHG_VDDP = 11,
	RT5081_ADC_TEMP_JC,
	RT5081_ADC_MAX,
};

/* Unit for each ADC parameter
 * 0 stands for reserved
 * For TS_BAT/TS_BUS, the real unit is 0.25.
 * Here we use 25, please remember to divide 100 while showing the value
 */
static const int rt5081_adc_unit[RT5081_ADC_MAX] = {
	0,
	RT5081_ADC_UNIT_VBUS_DIV5,
	RT5081_ADC_UNIT_VBUS_DIV2,
	RT5081_ADC_UNIT_VSYS,
	RT5081_ADC_UNIT_VBAT,
	0,
	RT5081_ADC_UNIT_TS_BAT,
	0,
	RT5081_ADC_UNIT_IBUS,
	RT5081_ADC_UNIT_IBAT,
	0,
	RT5081_ADC_UNIT_CHG_VDDP,
	RT5081_ADC_UNIT_TEMP_JC,
};

static const int rt5081_adc_offset[RT5081_ADC_MAX] = {
	0,
	RT5081_ADC_OFFSET_VBUS_DIV5,
	RT5081_ADC_OFFSET_VBUS_DIV2,
	RT5081_ADC_OFFSET_VSYS,
	RT5081_ADC_OFFSET_VBAT,
	0,
	RT5081_ADC_OFFSET_TS_BAT,
	0,
	RT5081_ADC_OFFSET_IBUS,
	RT5081_ADC_OFFSET_IBAT,
	0,
	RT5081_ADC_OFFSET_CHG_VDDP,
	RT5081_ADC_OFFSET_TEMP_JC,
};


/* =============================== */
/* rt5081 Charger Register Address */
/* =============================== */

static const unsigned char rt5081_chg_reg_addr[] = {
	RT5081_PMU_REG_CHGCTRL1,
	RT5081_PMU_REG_CHGCTRL2,
	RT5081_PMU_REG_CHGCTRL3,
	RT5081_PMU_REG_CHGCTRL4,
	RT5081_PMU_REG_CHGCTRL5,
	RT5081_PMU_REG_CHGCTRL6,
	RT5081_PMU_REG_CHGCTRL7,
	RT5081_PMU_REG_CHGCTRL8,
	RT5081_PMU_REG_CHGCTRL9,
	RT5081_PMU_REG_CHGCTRL10,
	RT5081_PMU_REG_CHGCTRL11,
	RT5081_PMU_REG_CHGCTRL12,
	RT5081_PMU_REG_CHGCTRL13,
	RT5081_PMU_REG_CHGCTRL14,
	RT5081_PMU_REG_CHGCTRL15,
	RT5081_PMU_REG_CHGCTRL16,
	RT5081_PMU_REG_CHGADC,
	RT5081_PMU_REG_DEVICETYPE,
	RT5081_PMU_REG_QCCTRL1,
	RT5081_PMU_REG_QCCTRL2,
	RT5081_PMU_REG_QC3P0CTRL1,
	RT5081_PMU_REG_QC3P0CTRL2,
	RT5081_PMU_REG_USBSTATUS1,
	RT5081_PMU_REG_QCSTATUS1,
	RT5081_PMU_REG_QCSTATUS2,
	RT5081_PMU_REG_CHGPUMP,
	RT5081_PMU_REG_CHGCTRL17,
	RT5081_PMU_REG_CHGCTRL18,
	RT5081_PMU_REG_CHGDIRCHG1,
	RT5081_PMU_REG_CHGDIRCHG2,
	RT5081_PMU_REG_CHGDIRCHG3,
	RT5081_PMU_REG_CHGSTAT,
	RT5081_PMU_REG_CHGNTC,
	RT5081_PMU_REG_ADCDATAH,
	RT5081_PMU_REG_ADCDATAL,
	RT5081_PMU_REG_CHGCTRL19,
};

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
static int rt_charger_set_ircmp_resistor(struct mtk_charger_info *mchr_info,
	void *data);
static int rt_charger_set_ircmp_vclamp(struct mtk_charger_info *mchr_info,
	void *data);

static inline void rt5081_irq_set_flag(
	struct rt5081_pmu_charger_data *chg_data, u8 *irq, u8 mask)
{
	mutex_lock(&chg_data->irq_access_lock);
	*irq |= mask;
	mutex_unlock(&chg_data->irq_access_lock);
}

static inline void rt5081_irq_clr_flag(
	struct rt5081_pmu_charger_data *chg_data, u8 *irq, u8 mask)
{
	mutex_lock(&chg_data->irq_access_lock);
	*irq &= ~mask;
	mutex_unlock(&chg_data->irq_access_lock);
}

static inline int rt5081_pmu_reg_test_bit(
	struct rt5081_pmu_chip *chip, u8 cmd, u8 shift)
{
	int ret = 0;

	ret = rt5081_pmu_reg_read(chip, cmd);
	if (ret < 0)
		return ret;

	ret = ret & (1 << shift);

	return ret;
}

static u8 rt5081_find_closest_reg_value(const u32 min, const u32 max,
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

static u8 rt5081_find_closest_reg_value_via_table(const u32 *value_table,
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

static u32 rt5081_find_closest_real_value(const u32 min, const u32 max,
	const u32 step, const u8 reg_val)
{
	u32 ret_val = 0;

	ret_val = min + reg_val * step;
	if (ret_val > max)
		ret_val = max;

	return ret_val;
}

static int rt5081_set_fast_charge_timer(
	struct rt5081_pmu_charger_data *chg_data, u32 hour)
{
	int ret = 0;
	u8 reg_fct = 0;

	battery_log(BAT_LOG_CRTI, "%s: set fast charge timer to %d\n",
		__func__, hour);

	reg_fct = rt5081_find_closest_reg_value(RT5081_WT_FC_MIN,
		RT5081_WT_FC_MAX, RT5081_WT_FC_STEP, RT5081_WT_FC_NUM, hour);

	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGCTRL12,
		RT5081_MASK_WT_FC,
		reg_fct << RT5081_SHIFT_WT_FC
	);

	return ret;
}


/* Hardware pin current limit */
static int rt5081_enable_ilim(struct rt5081_pmu_charger_data *chg_data,
	bool enable)
{
	int ret = 0;

	battery_log(BAT_LOG_CRTI, "%s: enable ilim = %d\n", __func__, enable);

	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGCTRL3, RT5081_MASK_ILIM_EN);

	return ret;
}

/* Select IINLMTSEL to use AICR */
static int rt5081_select_input_current_limit(
	struct rt5081_pmu_charger_data *chg_data, enum rt5081_iin_limit_sel sel)
{
	int ret = 0;

	battery_log(BAT_LOG_CRTI, "%s: select input current limit = %d\n",
		__func__, sel);

	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGCTRL2,
		RT5081_MASK_IINLMTSEL,
		sel << RT5081_SHIFT_IINLMTSEL
	);

	return ret;
}

/* Enter hidden mode */
static int rt5081_enable_hidden_mode(struct rt5081_pmu_charger_data *chg_data,
	bool enable)
{
	int ret = 0;

#if 0 /* TBD: Ask Thor */
	battery_log(BAT_LOG_CRTI, "%s: enable hidden mode = %d\n",
		__func__, enable);

	/* Disable hidden mode */
	if (!enable) {
		ret = rt5081_pmu_reg_write_(chg_data->chip, 0x70, 0x00);
		if (ret < 0)
			goto _err;
		return ret;
	}

	/* Enable hidden mode */
	ret = rt5081_pmu_reg_block_write(
		chg_data->chip,
		rt5081_reg_en_hidden_mode[0],
		ARRAY_SIZE(rt5081_val_en_hidden_mode),
		rt5081_val_en_hidden_mode
	);
	if (ret < 0)
		goto _err;

	return ret;

_err:
	battery_log(BAT_LOG_CRTI, "%s: enable hidden mode = %d failed\n",
		__func__, enable);
#endif
	return ret;
}

/* Software workaround */
static int rt5081_chg_sw_workaround(struct rt5081_pmu_charger_data *chg_data)
{
	int ret = 0;
#if 0

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	/* Enter hidden mode */
	ret = rt5081_enable_hidden_mode(chg_data, true);
	if (ret < 0)
		goto _out;

	/* Increase Isys drop threshold to 2.5A */
	ret = rt5081_pmu_reg_write(chg_data, 0x26, 0x1C);
	if (ret < 0)
		goto _out;

	/* Disable TS auto sensing */
	ret = rt5081_pmu_reg_clr_bit(chg_data, 0x2E, 0x01);

_out:
	/* Exit hidden mode */
	ret = rt5081_enable_hidden_mode(chg_data, false);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: exit hidden mode failed\n",
			__func__);

#endif
	return ret;
}

static int rt5081_get_adc(struct rt5081_pmu_charger_data *chg_data,
	enum rt5081_adc_sel adc_sel, int *adc_val)
{
	int ret = 0, i = 0;
	const int wait_retry = 5;
	u8 adc_data[2] = {0, 0};

	mutex_lock(&chg_data->adc_access_lock);

	/* Select ADC to desired channel */
	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGADC,
		RT5081_MASK_ADC_IN_SEL,
		adc_sel << RT5081_SHIFT_ADC_IN_SEL
	);

	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: select ADC to %d failed\n",
			__func__, adc_sel);
		goto _out;
	}

	/* Clear adc done event */
	rt5081_irq_clr_flag(chg_data, &chg_data->chg_stat6,
		RT5081_MASK_ADC_DONEI);

	/* Start ADC conversation */
	ret = rt5081_pmu_reg_set_bit(chg_data->chip, RT5081_PMU_REG_CHGADC,
		RT5081_MASK_ADC_START);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI,
			"%s: start ADC conversation failed, sel = %d\n",
			__func__, adc_sel);
		goto _out;
	}

	/* Wait for ADC conversation */
	for (i = 0; i < wait_retry; i++) {
		mdelay(35);
		if (chg_data->chg_stat6 & RT5081_MASK_ADC_DONEI) {
			battery_log(BAT_LOG_FULL,
				"%s: sel = %d, wait_times = %d\n",
				__func__, adc_sel, i);
			break;
		}
	}

	if (i == wait_retry) {
		battery_log(BAT_LOG_CRTI,
			"%s: Wait ADC conversation failed, sel = %d\n",
			__func__, adc_sel);
		ret = -EINVAL;
		goto _out;
	}

	/* Read ADC data high byte */
	ret = rt5081_pmu_reg_block_read(chg_data->chip, RT5081_PMU_REG_ADCDATAH,
		2, adc_data);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI,
			"%s: read ADC data failed\n", __func__);
		goto _out;
	}

	battery_log(BAT_LOG_FULL,
		"%s: adc_sel = %d, adc_h = 0x%02X, adc_l = 0x%02X\n",
		__func__, adc_sel, adc_data[0], adc_data[1]);

	/* Calculate ADC value */
	*adc_val = (adc_data[0] * 256 + adc_data[1]) * rt5081_adc_unit[adc_sel]
		+ rt5081_adc_offset[adc_sel];

_out:
	mutex_unlock(&chg_data->adc_access_lock);
	return ret;
}


static int rt5081_enable_watchdog(struct rt5081_pmu_charger_data *chg_data,
	bool enable)
{
	int ret = 0;

	battery_log(BAT_LOG_CRTI, "%s: enable watchdog = %d\n",
		__func__, enable);
	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGCTRL13, RT5081_MASK_WDT_EN);

	return ret;
}

static int rt5081_is_charging_enable(struct rt5081_pmu_charger_data *chg_data,
	bool *enable)
{
	int ret = 0;

	ret = rt5081_pmu_reg_read(chg_data->chip, RT5081_PMU_REG_CHGCTRL2);
	if (ret < 0)
		return ret;

	*enable = ((ret & RT5081_MASK_CHG_EN) >> RT5081_SHIFT_CHG_EN) > 0 ?
		true : false;

	return ret;
}

static int rt5081_enable_te(struct rt5081_pmu_charger_data *chg_data,
	bool enable)
{
	int ret = 0;

	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGCTRL2, RT5081_MASK_TE_EN);

	return ret;
}

static int rt5081_enable_pump_express(struct rt5081_pmu_charger_data *chg_data,
	bool enable)
{
	int ret = 0, i = 0;
	const int max_retry_times = 5;

	battery_log(BAT_LOG_CRTI, "%s: enable pumpX = %d\n", __func__, enable);

	/* Clear pumpx done event */
	rt5081_irq_clr_flag(chg_data, &chg_data->chg_stat6,
		RT5081_MASK_PUMPX_DONEI);

	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGCTRL17, RT5081_MASK_PUMPX_EN);
	if (ret < 0)
		return ret;

	for (i = 0; i < max_retry_times; i++) {
		msleep(2500);
		if (chg_data->chg_stat6 & RT5081_MASK_PUMPX_DONEI)
			break;
	}

	if (i == max_retry_times) {
		battery_log(BAT_LOG_CRTI, "%s: pumpX wait failed\n", __func__);
		ret = -EIO;
	}

	return ret;
}

static int rt5081_get_ieoc(struct rt5081_pmu_charger_data *chg_data, u32 *ieoc)
{
	int ret = 0;
	u8 reg_ieoc = 0;

	ret = rt5081_pmu_reg_read(chg_data->chip, RT5081_PMU_REG_CHGCTRL9);
	if (ret < 0)
		return ret;

	reg_ieoc = (ret & RT5081_MASK_IEOC) >> RT5081_SHIFT_IEOC;
	*ieoc = rt5081_find_closest_real_value(RT5081_IEOC_MIN, RT5081_IEOC_MAX,
		RT5081_IEOC_STEP, reg_ieoc);

	return ret;
}

static int rt5081_get_mivr(struct rt5081_pmu_charger_data *chg_data, u32 *mivr)
{
	int ret = 0;
	u8 reg_mivr = 0;

	ret = rt5081_pmu_reg_read(chg_data->chip, RT5081_PMU_REG_CHGCTRL6);
	if (ret < 0)
		return ret;
	reg_mivr = ((ret & RT5081_MASK_MIVR) >> RT5081_SHIFT_MIVR) & 0xFF;

	*mivr = rt5081_find_closest_real_value(RT5081_MIVR_MIN, RT5081_MIVR_MAX,
		RT5081_MIVR_STEP, reg_mivr);

	return ret;
}

static int rt5081_set_ieoc(struct rt5081_pmu_charger_data *chg_data, const u32 ieoc)
{
	int ret = 0;

	/* Find corresponding reg value */
	u8 reg_ieoc = rt5081_find_closest_reg_value(RT5081_IEOC_MIN,
		RT5081_IEOC_MAX, RT5081_IEOC_STEP, RT5081_IEOC_NUM, ieoc);

	battery_log(BAT_LOG_CRTI, "%s: ieoc = %d\n", __func__, ieoc);

	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGCTRL9,
		RT5081_MASK_IEOC,
		reg_ieoc << RT5081_SHIFT_IEOC
	);

	return ret;
}

static int rt5081_get_charging_status(struct rt5081_pmu_charger_data *chg_data,
	enum rt5081_charging_status *chg_stat)
{
	int ret = 0;

	ret = rt5081_pmu_reg_read(chg_data->chip, RT5081_PMU_REG_CHGSTAT);
	if (ret < 0)
		return ret;

	*chg_stat = (ret & RT5081_MASK_CHG_STAT) >> RT5081_SHIFT_CHG_STAT;

	return ret;
}

static int rt5081_set_dc_watchdog_timer(
	struct rt5081_pmu_charger_data *chg_data, u32 ms)
{
	int ret = 0;
	u8 reg_wdt = 0;

	reg_wdt = rt5081_find_closest_reg_value_via_table(
		rt5081_dc_watchdog_timer,
		ARRAY_SIZE(rt5081_dc_watchdog_timer),
		ms
	);

	battery_log(BAT_LOG_CRTI, "%s: dc watch dog timer = %dms(0x%02X)\n",
		__func__, ms, reg_wdt);

	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGDIRCHG2,
		RT5081_MASK_DC_WDT,
		reg_wdt << RT5081_SHIFT_DC_WDT
	);

	return ret;
}

static int rt5081_chg_sw_init(struct rt5081_pmu_charger_data *chg_data)
{
	int ret = 0;
	struct rt5081_pmu_charger_desc *chg_desc = chg_data->chg_desc;
	bool enable_safety_timer = true;
#ifdef CONFIG_MTK_BIF_SUPPORT
	u32 ircmp_resistor = 0;
	u32 ircmp_vclamp = 0;
#endif

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	/* Disable hardware ILIM */
	ret = rt5081_enable_ilim(chg_data, false);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI,
			"%s: Disable ilim failed\n", __func__);

	/* Select IINLMTSEL to use AICR */
	ret = rt5081_select_input_current_limit(chg_data,
		RT5081_IINLMTSEL_AICR);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: select iinlmtsel failed\n",
			__func__);

	ret = rt_charger_set_ichg(&chg_data->mchr_info, &chg_desc->ichg);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: Set ichg failed\n", __func__);

	ret = rt_charger_set_aicr(&chg_data->mchr_info, &chg_desc->aicr);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: Set aicr failed\n", __func__);

	ret = rt_charger_set_mivr(&chg_data->mchr_info, &chg_desc->mivr);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: Set mivr failed\n", __func__);

	ret = rt_charger_set_battery_voreg(&chg_data->mchr_info, &chg_desc->cv);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: set voreg failed\n", __func__);

	ret = rt5081_set_ieoc(chg_data, chg_desc->ieoc); /* mA */
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: Set ieoc failed\n", __func__);

	/* Enable TE */
	ret = rt5081_enable_te(chg_data, chg_desc->enable_te);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: Set te failed\n", __func__);

	/* Set fast charge timer */
	ret = rt5081_set_fast_charge_timer(chg_data, chg_desc->safety_timer);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI,
			"%s: Set fast timer failed\n", __func__);

	/* Set DC watch dog timer */
	ret = rt5081_set_dc_watchdog_timer(chg_data, chg_desc->dc_wdt);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: Set dc watch dog timer failed\n",
			__func__);

	/* Enable charger timer */
	ret = rt_charger_enable_safety_timer(&chg_data->mchr_info,
		&enable_safety_timer);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: enable charger timer failed\n",
			__func__);

	/* Enable watchdog timer */
	ret = rt5081_enable_watchdog(chg_data, true);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: enable watchdog timer failed\n",
			__func__);

	/* Set ircomp according to BIF */
#ifdef CONFIG_MTK_BIF_SUPPORT
	ret = rt_charger_set_ircmp_resistor(&chg_data->mchr_info,
		&ircmp_resistor);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI,
			"%s: set IR compensation resistor failed\n", __func__);

	ret = rt_charger_set_ircmp_vclamp(&chg_data->mchr_info, &ircmp_vclamp);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI,
			"%s: set IR compensation voltage clamp failed\n",
			__func__);
#else
	ret = rt_charger_set_ircmp_resistor(&chg_data->mchr_info,
		&chg_desc->ircmp_resistor);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI,
			"%s: set IR compensation resistor failed\n", __func__);

	ret = rt_charger_set_ircmp_vclamp(&chg_data->mchr_info,
		&chg_desc->ircmp_vclamp);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI,
			"%s: set IR compensation voltage clamp failed\n",
			__func__);
#endif

	return ret;
}

/* Set register's value to default */
static int rt5081_chg_hw_init(struct rt5081_pmu_charger_data *chg_data)
{
	int ret = 0;

#if 0 /* TBD */
	battery_log(BAT_LOG_FULL, "%s: starts\n", __func__);

	ret = rt5081_pmu_reg_set_bit(chg_data->chip, RT5081_REG_CORE_CTRL0,
		RT5081_MASK_RST);

#endif
	return ret;
}

static int rt5081_set_aicl_vth(struct rt5081_pmu_charger_data *chg_data,
	u32 aicl_vth)
{
	int ret = 0;
	u8 reg_aicl_vth = 0;

	battery_log(BAT_LOG_CRTI, "%s: aicl_vth = %d\n", __func__, aicl_vth);

	reg_aicl_vth = rt5081_find_closest_reg_value(RT5081_AICL_VTH_MIN,
		RT5081_AICL_VTH_MAX, RT5081_AICL_VTH_STEP, RT5081_AICL_VTH_NUM,
		aicl_vth);

	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGCTRL14,
		RT5081_MASK_AICL_VTH,
		reg_aicl_vth << RT5081_SHIFT_AICL_VTH
	);

	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: set aicl vth failed, ret = %d\n",
			__func__, ret);

	return ret;
}

static void rt5081_discharging_work_handler(struct work_struct *work)
{
#if 0 /* TBD: ask thor */
	int ret = 0, i = 0;
	const unsigned int max_retry_cnt = 3;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)container_of(work,
		struct rt5081_pmu_charger_data, discharging_work);

	ret = rt5081_enable_hidden_mode(chg_data, true);
	if (ret < 0)
		return;

	battery_log(BAT_LOG_CRTI, "%s: start discharging\n", __func__);

	/* Set bit2 of reg[0x21] to 1 to enable discharging */
	ret = rt5081_pmu_reg_set_bit(chg_data, 0x21, 0x04);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: enable discharging failed\n",
			__func__);
		goto _out;
	}

	/* Wait for discharging to 0V */
	mdelay(20);

	for (i = 0; i < max_retry_cnt; i++) {
		/* Disable discharging */
		ret = rt5081_pmu_reg_clr_bit(chg_data, 0x21, 0x04);
		if (ret < 0)
			continue;
		if (rt5081_i2c_test_bit(chg_data, 0x21, 2) == 0)
			goto _out;
	}

	battery_log(BAT_LOG_CRTI, "%s: disable discharging failed\n",
		__func__);
_out:
	rt5081_enable_hidden_mode(chg_data, false);
	battery_log(BAT_LOG_CRTI, "%s: end discharging\n", __func__);
#endif
}

static int rt5081_run_discharging(struct rt5081_pmu_charger_data *chg_data)
{
#if 0
	if (!queue_work(chg_data->discharging_workqueue,
		&info->discharging_work))
		return -EINVAL;

#endif
	return 0;
}

static int rt5081_get_battery_voreg(struct rt5081_pmu_charger_data *chg_data,
	u32 *voreg)
{
	int ret = 0;
	u8 reg_voreg = 0;

	ret = rt5081_pmu_reg_read(chg_data->chip, RT5081_PMU_REG_CHGCTRL4);
	if (ret < 0)
		return ret;

	reg_voreg = (ret & RT5081_MASK_BAT_VOREG) >> RT5081_SHIFT_BAT_VOREG;
	*voreg = rt5081_find_closest_real_value(RT5081_BAT_VOREG_MIN,
		RT5081_BAT_VOREG_MAX, RT5081_BAT_VOREG_STEP, reg_voreg);

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
	int adc_vsys = 0, adc_vbat = 0, adc_ibat = 0, adc_ibus = 0;
	enum rt5081_charging_status chg_status = RT5081_CHG_STATUS_READY;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	ret = rt_charger_get_ichg(mchr_info, &ichg); /* 10uA */
	ret = rt_charger_get_aicr(mchr_info, &aicr); /* 10uA */
	ret = rt5081_get_charging_status(chg_data, &chg_status);
	ret = rt5081_get_ieoc(chg_data, &ieoc);
	ret = rt5081_get_mivr(chg_data, &mivr);
	ret = rt5081_is_charging_enable(chg_data, &chg_enable);
	ret = rt5081_get_adc(chg_data, RT5081_ADC_VSYS, &adc_vsys);
	ret = rt5081_get_adc(chg_data, RT5081_ADC_VBAT, &adc_vbat);
	ret = rt5081_get_adc(chg_data, RT5081_ADC_IBAT, &adc_ibat);
	ret = rt5081_get_adc(chg_data, RT5081_ADC_IBUS, &adc_ibus);

	if ((Enable_BATDRV_LOG > BAT_LOG_CRTI) ||
	    (chg_status == RT5081_CHG_STATUS_FAULT)) {
		chg_data->i2c_log_level = BAT_LOG_CRTI;
		for (i = 0; i < ARRAY_SIZE(rt5081_chg_reg_addr); i++) {
			ret = rt5081_pmu_reg_read(chg_data->chip,
				rt5081_chg_reg_addr[i]);
			if (ret < 0)
				return ret;
		}
	} else
		chg_data->i2c_log_level = BAT_LOG_FULL;

	battery_log(BAT_LOG_CRTI,
		"%s: ICHG = %dmA, AICR = %dmA, MIVR = %dmV, IEOC = %dmA\n",
		__func__, ichg / 100, aicr / 100, mivr, ieoc);

	battery_log(BAT_LOG_CRTI,
		"%s: VSYS = %dmV, VBAT = %dmV IBAT = %dmA, IBUS = %dmA\n",
		__func__, adc_vsys, adc_vbat, adc_ibat, adc_ibus);

	battery_log(BAT_LOG_CRTI,
		"%s: CHG_EN = %d, CHG_STATUS = %s\n",
		__func__, chg_enable, rt5081_chg_status_name[chg_status]);

	return ret;
}

static int rt_charger_enable_charging(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u8 enable = *((u8 *)data);
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGCTRL2, RT5081_MASK_CHG_EN);

	return ret;
}

static int rt_charger_enable_hz(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u8 enable = *((u8 *)data);
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGCTRL1, RT5081_MASK_CHG_EN);

	return ret;
}

static int rt_charger_enable_safety_timer(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u8 enable = *((u8 *)data);
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGCTRL12, RT5081_MASK_TMR_EN);

	return ret;
}

static int rt_charger_enable_otg(struct mtk_charger_info *mchr_info, void *data)
{
	u8 enable = *((u8 *)data);
	int ret = 0, i = 0;
	const int ss_wait_times = 5; /* soft start wait times */
	u32 current_limit = 500; /* mA */
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	/* Set OTG_OC to 500mA */
	ret = rt_charger_set_boost_current_limit(mchr_info, &current_limit);
	if (ret < 0)
		return ret;

	/* Switch OPA mode to boost mode */
	battery_log(BAT_LOG_CRTI, "%s: otg enable = %d\n", __func__, enable);

	/* Clear soft start event */
	rt5081_irq_clr_flag(chg_data, &chg_data->chg_stat5,
		RT5081_MASK_CHG_SSFINISHI);

	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGCTRL1, RT5081_MASK_OPA_MODE);

	if (!enable) {
		/* Enter hidden mode */
		ret = rt5081_enable_hidden_mode(chg_data, true);
		if (ret < 0)
			return ret;

		/* Workaround: reg[0x25] = 0x0F after leaving OTG mode */
		ret = rt5081_pmu_reg_write(chg_data->chip, 0x25, 0x0F);
		if (ret < 0)
			battery_log(BAT_LOG_CRTI, "%s: otg workaroud failed\n",
				__func__);

		/* Exit hidden mode */
		ret = rt5081_enable_hidden_mode(chg_data, false);

		ret = rt5081_run_discharging(chg_data);
		if (ret < 0)
			battery_log(BAT_LOG_CRTI, "%s: run discharging failed\n",
				__func__);

		return ret;
	}

	/* Wait for soft start finish interrupt */
	for (i = 0; i < ss_wait_times; i++) {
		mdelay(100);
		if (chg_data->chg_stat5 & RT5081_MASK_CHG_SSFINISHI) {
			/* Enter hidden mode */
			ret = rt5081_enable_hidden_mode(chg_data, true);
			if (ret < 0)
				return ret;

			/* Woraround reg[0x25] = 0x00 after entering OTG mode */
			ret = rt5081_pmu_reg_write(chg_data->chip, 0x25, 0x00);
			if (ret < 0)
				battery_log(BAT_LOG_CRTI,
					"%s: otg workaroud failed\n", __func__);

			/* Exit hidden mode */
			ret = rt5081_enable_hidden_mode(chg_data, false);
			battery_log(BAT_LOG_CRTI,
				"%s: otg soft start successfully\n", __func__);

			break;
		}
	}

	if (i == ss_wait_times) {
		battery_log(BAT_LOG_CRTI,
				"%s: otg soft start failed\n", __func__);
		/* Disable OTG */
		rt5081_pmu_reg_clr_bit(chg_data->chip, RT5081_PMU_REG_CHGCTRL1,
				RT5081_MASK_OPA_MODE);
		ret = -EIO;
	}

	return ret;
}

static int rt_charger_enable_power_path(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u8 enable = *((u8 *)data);
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	/* Use HZ mode instead of turning off CFO */
	ret = (enable ? rt5081_pmu_reg_clr_bit : rt5081_pmu_reg_set_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGCTRL1, RT5081_MASK_HZ_EN);

	return ret;
}

static int rt_charger_enable_direct_charge(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u8 enable = *((u8 *)data);
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	battery_log(BAT_LOG_CRTI, "%s: enable direct charger = %d\n",
		__func__, enable);

	if (enable) {
		/* Enable bypass mode */
		ret = rt5081_pmu_reg_set_bit(chg_data->chip,
			RT5081_PMU_REG_CHGCTRL2, RT5081_MASK_BYPASS_MODE);
		if (ret < 0) {
			battery_log(BAT_LOG_CRTI, "%s: enable bypass mode failed\n", __func__);
			return ret;
		}

		/* VG_EN = 1 */
		ret = rt5081_pmu_reg_set_bit(chg_data->chip,
			RT5081_PMU_REG_CHGPUMP, RT5081_MASK_VG_EN);
		if (ret < 0)
			battery_log(BAT_LOG_CRTI, "%s: enable VG_EN failed\n", __func__);

		return ret;
	}

	/* Disable direct charge */
	/* VG_EN = 0 */
	ret = rt5081_pmu_reg_clr_bit(chg_data->chip, RT5081_PMU_REG_CHGPUMP,
		RT5081_MASK_VG_EN);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: disable VG_EN failed\n", __func__);

	/* Disable bypass mode */
	ret = rt5081_pmu_reg_clr_bit(chg_data->chip, RT5081_PMU_REG_CHGCTRL2,
		RT5081_MASK_BYPASS_MODE);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: disable bypass mode failed\n", __func__);

	return ret;
}

static int rt_charger_set_ichg(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u32 ichg = 0;
	u8 reg_ichg = 0;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	/* MTK's current unit : 10uA */
	/* Our current unit : mA */
	ichg = *((u32 *)data);
	ichg /= 100;

	/* Find corresponding reg value */
	reg_ichg = rt5081_find_closest_reg_value(RT5081_ICHG_MIN,
		RT5081_ICHG_MAX, RT5081_ICHG_STEP, RT5081_ICHG_NUM, ichg);

	battery_log(BAT_LOG_CRTI, "%s: ichg = %d\n", __func__, ichg);

	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGCTRL7,
		RT5081_MASK_ICHG,
		reg_ichg << RT5081_SHIFT_ICHG
	);

	return ret;
}

static int rt_charger_set_aicr(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u32 aicr = 0;
	u8 reg_aicr = 0;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	/* MTK's current unit : 10uA */
	/* Our current unit : mA */
	aicr = *((u32 *)data);
	aicr /= 100;

	/* Find corresponding reg value */
	reg_aicr = rt5081_find_closest_reg_value(RT5081_AICR_MIN,
		RT5081_AICR_MAX, RT5081_AICR_STEP, RT5081_AICR_NUM, aicr);

	battery_log(BAT_LOG_CRTI, "%s: aicr = %d\n", __func__, aicr);

	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGCTRL3,
		RT5081_MASK_AICR,
		reg_aicr << RT5081_SHIFT_AICR
	);

	return ret;
}


static int rt_charger_set_mivr(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u32 mivr = *((u32 *)data);
	u8 reg_mivr = 0;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	/* Find corresponding reg value */
	reg_mivr = rt5081_find_closest_reg_value(RT5081_MIVR_MIN,
		RT5081_MIVR_MAX, RT5081_MIVR_STEP, RT5081_MIVR_NUM, mivr);

	battery_log(BAT_LOG_CRTI, "%s: mivr = %d\n", __func__, mivr);

	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGCTRL6,
		RT5081_MASK_MIVR,
		reg_mivr << RT5081_SHIFT_MIVR
	);

	return ret;
}

static int rt_charger_set_battery_voreg(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u32 voreg = 0;
	u8 reg_voreg = 0;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	/* MTK's voltage unit : uV */
	/* Our voltage unit : mV */
	voreg = *((u32 *)data);
	voreg /= 1000;

	reg_voreg = rt5081_find_closest_reg_value(RT5081_BAT_VOREG_MIN,
		RT5081_BAT_VOREG_MAX, RT5081_BAT_VOREG_STEP,
		RT5081_BAT_VOREG_NUM, voreg);

	battery_log(BAT_LOG_CRTI, "%s: bat voreg = %d\n", __func__, voreg);

	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGCTRL4,
		RT5081_MASK_BAT_VOREG,
		reg_voreg << RT5081_SHIFT_BAT_VOREG
	);

	return ret;
}


static int rt_charger_set_boost_current_limit(
	struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u8 reg_ilimit = 0;
	u32 current_limit = *((u32 *)data);
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	reg_ilimit = rt5081_find_closest_reg_value_via_table(
		rt5081_otg_oc_threshold,
		ARRAY_SIZE(rt5081_otg_oc_threshold),
		current_limit
	);

	battery_log(BAT_LOG_CRTI, "%s: boost ilimit = %d\n",
		__func__, current_limit);

	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGCTRL10,
		RT5081_MASK_BOOST_OC,
		reg_ilimit << RT5081_SHIFT_BOOST_OC
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
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	battery_log(BAT_LOG_CRTI, "%s: pe1.0 pump_up = %d\n",
		__func__, is_pump_up);

	/* Set to PE1.0 */
	ret = rt5081_pmu_reg_clr_bit(chg_data->chip, RT5081_PMU_REG_CHGCTRL17,
		RT5081_MASK_PUMPX_20_10);

	/* Set Pump Up/Down */
	ret = (is_pump_up ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGCTRL17,
		RT5081_MASK_PUMPX_UP_DN);
	if (ret < 0)
		return ret;

	ret = rt_charger_set_aicr(mchr_info, &aicr);
	if (ret < 0)
		return ret;

	ret = rt_charger_set_ichg(mchr_info, &ichg);
	if (ret < 0)
		return ret;

	/* Enable PumpX */
	ret = rt5081_enable_pump_express(chg_data, true);

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

static int rt_charger_set_pep20_current_pattern(
	struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u8 reg_volt = 0;
	u32 voltage = *((u32 *)data);
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	battery_log(BAT_LOG_CRTI, "%s: pep2.0  = %d\n", __func__, voltage);

	/* Find register value of target voltage */
	reg_volt = rt5081_find_closest_reg_value(RT5081_PEP20_VOLT_MIN,
		RT5081_PEP20_VOLT_MAX, RT5081_PEP20_VOLT_STEP,
		RT5081_PEP20_VOLT_NUM, voltage);

	/* Set to PEP2.0 */
	ret = rt5081_pmu_reg_set_bit(chg_data->chip, RT5081_PMU_REG_CHGCTRL17,
		RT5081_MASK_PUMPX_20_10);
	if (ret < 0)
		return ret;

	/* Set Voltage */
	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGCTRL17,
		RT5081_MASK_PUMPX_DEC,
		reg_volt << RT5081_SHIFT_PUMPX_DEC
	);
	if (ret < 0)
		return ret;

	/* Enable PumpX */
	ret = rt5081_enable_pump_express(chg_data, true);

	return ret;
}

static int rt_charger_get_ichg(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0;
	u8 reg_ichg = 0;
	u32 ichg = 0;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	ret = rt5081_pmu_reg_read(chg_data->chip, RT5081_PMU_REG_CHGCTRL7);
	if (ret < 0)
		return ret;

	reg_ichg = (ret & RT5081_MASK_ICHG) >> RT5081_SHIFT_ICHG;
	ichg = rt5081_find_closest_real_value(RT5081_ICHG_MIN, RT5081_ICHG_MAX,
		RT5081_ICHG_STEP, reg_ichg);

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
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	ret = rt5081_pmu_reg_read(chg_data->chip, RT5081_PMU_REG_CHGCTRL3);
	if (ret < 0)
		return ret;

	reg_aicr = (ret & RT5081_MASK_AICR) >> RT5081_SHIFT_AICR;
	aicr = rt5081_find_closest_real_value(RT5081_AICR_MIN, RT5081_AICR_MAX,
		RT5081_AICR_STEP, reg_aicr);

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
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	/* Get value from ADC */
	ret = rt5081_get_adc(chg_data, RT5081_ADC_TEMP_JC, &adc_temp);
	if (ret < 0)
		return ret;

	((int *)data)[0] = adc_temp;
	((int *)data)[1] = adc_temp;

	battery_log(BAT_LOG_CRTI, "%s: temperature = %d\n", __func__, adc_temp);

	return ret;
}

static int rt_charger_get_ibus(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0, adc_ibus = 0;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	/* Get value from ADC */
	ret = rt5081_get_adc(chg_data, RT5081_ADC_IBUS, &adc_ibus);
	if (ret < 0)
		return ret;

	*((u32 *)data) = adc_ibus;

	battery_log(BAT_LOG_CRTI, "%s: ibus = %dmA\n", __func__, adc_ibus);
	return ret;
}

static int rt_charger_get_vbus(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0, adc_vbus = 0;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	/* Get value from ADC */
	ret = rt5081_get_adc(chg_data, RT5081_ADC_VBUS_DIV2, &adc_vbus);
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
	enum rt5081_charging_status chg_stat = RT5081_CHG_STATUS_READY;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	ret = rt5081_get_charging_status(chg_data, &chg_stat);

	/* Return is charging done or not */
	switch (chg_stat) {
	case RT5081_CHG_STATUS_READY:
	case RT5081_CHG_STATUS_PROGRESS:
	case RT5081_CHG_STATUS_FAULT:
		*((u32 *)data) = 0;
		break;
	case RT5081_CHG_STATUS_DONE:
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
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	ret = rt5081_pmu_reg_test_bit(chg_data->chip, RT5081_PMU_REG_CHGCTRL1,
		RT5081_SHIFT_HZ_EN);
	if (ret < 0)
		return ret;

	*((bool *)data) = (ret > 0) ? false : true;

	return ret;
}

static int rt_charger_is_safety_timer_enable(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	ret = rt5081_pmu_reg_test_bit(chg_data->chip, RT5081_PMU_REG_CHGCTRL12,
		RT5081_SHIFT_TMR_EN);
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
	enum rt5081_charging_status chg_status;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	ret = rt5081_get_charging_status(chg_data, &chg_status);

	return ret;
}

static int rt_charger_run_aicl(struct mtk_charger_info *mchr_info, void *data)
{
	int ret = 0, i = 0;
	u32 mivr = 0, aicl_vth = 0, aicr = 0;
	const u32 max_wait_time = 5;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;
	bool chg_mivr_event = false;

	/* Check whether MIVR loop is active */
	chg_mivr_event =
		(chg_data->chg_stat1 & RT5081_MASK_CHG_MIVR) ?
		true : false;
	if (!chg_mivr_event) {
		battery_log(BAT_LOG_CRTI, "%s: mivr loop is not active\n",
			__func__);
		return 0;
	}
	battery_log(BAT_LOG_CRTI, "%s: mivr loop is active\n", __func__);

	/* Clear chg mivr event */
	rt5081_irq_clr_flag(chg_data, &chg_data->chg_stat1,
		RT5081_MASK_CHG_MIVR);

	ret = rt5081_get_mivr(chg_data, &mivr);
	if (ret < 0)
		goto _out;

	/* Check if there's a suitable AICL_VTH */
	aicl_vth = mivr + 200;
	if (aicl_vth > RT5081_AICL_VTH_MAX) {
		battery_log(BAT_LOG_CRTI, "%s: no suitable AICL_VTH, vth = %d\n",
			__func__, aicl_vth);
		ret = -EINVAL;
		goto _out;
	}

	ret = rt5081_set_aicl_vth(chg_data, aicl_vth);
	if (ret < 0)
		goto _out;

	rt5081_irq_clr_flag(chg_data, &chg_data->chg_stat5,
		RT5081_MASK_CHG_AICLMEASI);
	ret = rt5081_pmu_reg_set_bit(chg_data->chip, RT5081_PMU_REG_CHGCTRL14,
		RT5081_MASK_AICL_MEAS);
	if (ret < 0)
		goto _out;

	for (i = 0; i < max_wait_time; i++) {
		msleep(500);
		if (chg_data->chg_stat5 & RT5081_MASK_CHG_AICLMEASI)
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

static int rt_charger_reset_dc_watchdog_timer(
	struct mtk_charger_info *mchr_info, void *data)
{
	/* Any I2C communication can reset watchdog timer */
	int ret = 0;
	enum rt5081_charging_status chg_status;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	ret = rt5081_get_charging_status(chg_data, &chg_status);

	return ret;
}

static int rt_charger_enable_dc_vbusov(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	bool enable = *((bool *)data);
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGDIRCHG3,
		RT5081_MASK_DC_VBUSOV_EN);

	return ret;
}

static int rt_charger_set_dc_vbusov(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u8 reg_vbusov = 0;
	u32 vbusov = *((u32 *)data);
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	battery_log(BAT_LOG_CRTI, "%s: set dc vbusov = %dmV\n", __func__,
		vbusov);

	reg_vbusov = rt5081_find_closest_reg_value(RT5081_DC_VBUSOV_LVL_MIN,
		RT5081_DC_VBUSOV_LVL_MAX, RT5081_DC_VBUSOV_LVL_STEP,
		RT5081_DC_VBUSOV_LVL_NUM, vbusov);

	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGDIRCHG3,
		RT5081_MASK_DC_VBUSOV_LVL,
		reg_vbusov << RT5081_SHIFT_DC_VBUSOV_LVL
	);

	return ret;
}

static int rt_charger_enable_dc_ibusoc(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	bool enable = *((bool *)data);
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	battery_log(BAT_LOG_CRTI, "%s: enable dc vbusoc = %d\n", __func__,
		enable);
	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGDIRCHG1,
		RT5081_MASK_DC_IBUSOC_EN);

	return ret;
}

static int rt_charger_set_dc_ibusoc(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u8 reg_ibusoc = 0;
	u32 ibusoc = *((u32 *)data);
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	battery_log(BAT_LOG_CRTI, "%s: set dc ibusoc = %dmA\n", __func__,
		ibusoc);

	reg_ibusoc = rt5081_find_closest_reg_value(RT5081_DC_IBUSOC_LVL_MIN,
		RT5081_DC_IBUSOC_LVL_MAX, RT5081_DC_IBUSOC_LVL_STEP,
		RT5081_DC_IBUSOC_LVL_NUM, ibusoc);

	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGDIRCHG1,
		RT5081_MASK_DC_IBUSOC_LVL,
		reg_ibusoc << RT5081_SHIFT_DC_IBUSOC_LVL
	);

	return ret;
}

static int rt_charger_enable_dc_vbatov(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	bool enable = *((bool *)data);
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	battery_log(BAT_LOG_CRTI, "%s: enable dc vbusov = %d\n", __func__,
		enable);
	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGDIRCHG1,
		RT5081_MASK_DC_VBATOV_EN);

	return ret;
}

static int rt_charger_set_dc_vbatov(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0, i = 0;
	u8 reg_vbatov = 0;
	u32 voreg = 0, vbatov = 0;
	u32 target_vbatov = *((u32 *)data);
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;


	battery_log(BAT_LOG_CRTI, "%s: set dc vbatov = %dmV\n", __func__,
		vbatov);

	ret = rt5081_get_battery_voreg(chg_data, &voreg);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: get voreg failed\n", __func__);
		return ret;
	}

	for (i = 0; i < ARRAY_SIZE(rt5081_dc_vbatov_lvl); i++) {
		vbatov = (rt5081_dc_vbatov_lvl[i] * voreg) / 100;

		/* Choose closest level */
		if (target_vbatov <= vbatov) {
			reg_vbatov = i;
			break;
		}
	}
	if (i == ARRAY_SIZE(rt5081_dc_vbatov_lvl))
		reg_vbatov = i;

	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGDIRCHG1,
		RT5081_MASK_DC_VBATOV_LVL,
		reg_vbatov << RT5081_SHIFT_DC_VBATOV_LVL
	);

	return ret;
}

static int rt_charger_is_dc_enable(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	ret = rt5081_pmu_reg_test_bit(chg_data->chip, RT5081_PMU_REG_CHGPUMP,
		RT5081_SHIFT_VG_EN);
	if (ret < 0)
		return ret;

	*((bool *)data) = (ret > 0) ? true : false;

	return ret;
}

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

static int rt_charger_set_ircmp_resistor(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u32 resistor = 0;
	u8 reg_resistor = 0;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	resistor = *((u32 *)data);
	battery_log(BAT_LOG_CRTI, "%s: set ir comp resistor = %d\n",
		__func__, resistor);

	reg_resistor = rt5081_find_closest_reg_value(RT5081_IRCMP_RES_MIN,
		RT5081_IRCMP_RES_MAX, RT5081_IRCMP_RES_STEP, RT5081_IRCMP_RES_NUM,
		resistor);

	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGCTRL18,
		RT5081_MASK_IRCMP_RES,
		reg_resistor << RT5081_SHIFT_IRCMP_RES
	);

	return ret;
}

static int rt_charger_set_ircmp_vclamp(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u32 vclamp = 0;
	u8 reg_vclamp = 0;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	vclamp = *((u32 *)data);
	battery_log(BAT_LOG_CRTI, "%s: set ir comp vclamp = %d\n",
		__func__, vclamp);

	reg_vclamp = rt5081_find_closest_reg_value(RT5081_IRCMP_VCLAMP_MIN,
		RT5081_IRCMP_VCLAMP_MAX, RT5081_IRCMP_VCLAMP_STEP,
		RT5081_IRCMP_VCLAMP_NUM, vclamp);

	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGCTRL18,
		RT5081_MASK_IRCMP_VCLAMP,
		reg_vclamp << RT5081_SHIFT_IRCMP_VCLAMP
	);

	return ret;
}

static int rt_charger_set_error_state(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	chg_data->err_state = *((bool *)data);
	rt_charger_enable_hz(mchr_info, &chg_data->err_state);

	return ret;
}

static const mtk_charger_intf rt5081_mchr_intf[CHARGING_CMD_NUMBER] = {
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
	[CHARGING_CMD_RESET_DC_WATCH_DOG_TIMER] = rt_charger_reset_dc_watchdog_timer,
	[CHARGING_CMD_ENABLE_DC_VBUSOV] = rt_charger_enable_dc_vbusov,
	[CHARGING_CMD_SET_DC_VBUSOV] = rt_charger_set_dc_vbusov,
	[CHARGING_CMD_ENABLE_DC_VBUSOC] = rt_charger_enable_dc_ibusoc,
	[CHARGING_CMD_SET_DC_VBUSOC] = rt_charger_set_dc_ibusoc,
	[CHARGING_CMD_ENABLE_DC_VBATOV] = rt_charger_enable_dc_vbatov,
	[CHARGING_CMD_SET_DC_VBATOV] = rt_charger_set_dc_vbatov,
	[CHARGING_CMD_GET_IS_DC_ENABLE] = rt_charger_is_dc_enable,
	[CHARGING_CMD_SET_IRCMP_RESISTOR] = rt_charger_set_ircmp_resistor,
	[CHARGING_CMD_SET_IRCMP_VOLT_CLAMP] = rt_charger_set_ircmp_vclamp,
	[CHARGING_CMD_SET_PEP20_EFFICIENCY_TABLE] = rt_charger_set_pep20_efficiency_table,

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


static irqreturn_t rt5081_pmu_chg_treg_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_aicr_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_mivr_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	rt5081_irq_set_flag(chg_data, &chg_data->chg_stat1,
		RT5081_MASK_CHG_MIVR);

	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_pwr_rdy_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_vinovp_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_vsysuv_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_vsysov_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_vbatov_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_vbusov_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ts_bat_cold_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ts_bat_cool_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ts_bat_warm_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ts_bat_hot_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_tmri_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_batabsi_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_adpbadi_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_rvpi_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_otpi_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_aiclmeasi_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	rt5081_irq_set_flag(chg_data, &chg_data->chg_stat5,
		RT5081_MASK_CHG_AICLMEASI);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_ichgmeasi_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chgdet_donei_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_wdtmri_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ssfinishi_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	rt5081_irq_set_flag(chg_data, &chg_data->chg_stat5,
		RT5081_MASK_CHG_SSFINISHI);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_rechgi_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_termi_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_ieoci_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_adc_donei_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	rt5081_irq_set_flag(chg_data, &chg_data->chg_stat6,
		RT5081_MASK_ADC_DONEI);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_pumpx_donei_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	rt5081_irq_set_flag(chg_data, &chg_data->chg_stat6,
		RT5081_MASK_PUMPX_DONEI);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_bst_batuvi_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_bst_vbusovi_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_bst_olpi_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_attachi_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_detachi_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_qc30stpdone_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_qc_vbusdet_done_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_hvdcp_det_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chgdeti_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_dcdti_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_dirchg_vgoki_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_dirchg_wdtmri_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_dirchg_uci_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_dirchg_oci_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_dirchg_ovi_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ovpctrl_swon_evt_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ovpctrl_uvp_d_evt_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ovpctrl_uvp_evt_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ovpctrl_ovp_d_evt_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ovpctrl_ovp_evt_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static struct rt5081_pmu_irq_desc rt5081_chg_irq_desc[] = {
	RT5081_PMU_IRQDESC(chg_treg),
	RT5081_PMU_IRQDESC(chg_aicr),
	RT5081_PMU_IRQDESC(chg_mivr),
	RT5081_PMU_IRQDESC(pwr_rdy),
	RT5081_PMU_IRQDESC(chg_vinovp),
	RT5081_PMU_IRQDESC(chg_vsysuv),
	RT5081_PMU_IRQDESC(chg_vsysov),
	RT5081_PMU_IRQDESC(chg_vbatov),
	RT5081_PMU_IRQDESC(chg_vbusov),
	RT5081_PMU_IRQDESC(ts_bat_cold),
	RT5081_PMU_IRQDESC(ts_bat_cool),
	RT5081_PMU_IRQDESC(ts_bat_warm),
	RT5081_PMU_IRQDESC(ts_bat_hot),
	RT5081_PMU_IRQDESC(chg_tmri),
	RT5081_PMU_IRQDESC(chg_batabsi),
	RT5081_PMU_IRQDESC(chg_adpbadi),
	RT5081_PMU_IRQDESC(chg_rvpi),
	RT5081_PMU_IRQDESC(otpi),
	RT5081_PMU_IRQDESC(chg_aiclmeasi),
	RT5081_PMU_IRQDESC(chg_ichgmeasi),
	RT5081_PMU_IRQDESC(chgdet_donei),
	RT5081_PMU_IRQDESC(chg_wdtmri),
	RT5081_PMU_IRQDESC(ssfinishi),
	RT5081_PMU_IRQDESC(chg_rechgi),
	RT5081_PMU_IRQDESC(chg_termi),
	RT5081_PMU_IRQDESC(chg_ieoci),
	RT5081_PMU_IRQDESC(adc_donei),
	RT5081_PMU_IRQDESC(pumpx_donei),
	RT5081_PMU_IRQDESC(bst_batuvi),
	RT5081_PMU_IRQDESC(bst_vbusovi),
	RT5081_PMU_IRQDESC(bst_olpi),
	RT5081_PMU_IRQDESC(attachi),
	RT5081_PMU_IRQDESC(detachi),
	RT5081_PMU_IRQDESC(qc30stpdone),
	RT5081_PMU_IRQDESC(qc_vbusdet_done),
	RT5081_PMU_IRQDESC(hvdcp_det),
	RT5081_PMU_IRQDESC(chgdeti),
	RT5081_PMU_IRQDESC(dcdti),
	RT5081_PMU_IRQDESC(dirchg_vgoki),
	RT5081_PMU_IRQDESC(dirchg_wdtmri),
	RT5081_PMU_IRQDESC(dirchg_uci),
	RT5081_PMU_IRQDESC(dirchg_oci),
	RT5081_PMU_IRQDESC(dirchg_ovi),
	RT5081_PMU_IRQDESC(ovpctrl_swon_evt),
	RT5081_PMU_IRQDESC(ovpctrl_uvp_d_evt),
	RT5081_PMU_IRQDESC(ovpctrl_uvp_evt),
	RT5081_PMU_IRQDESC(ovpctrl_ovp_d_evt),
	RT5081_PMU_IRQDESC(ovpctrl_ovp_evt),
};

static void rt5081_pmu_charger_irq_register(struct platform_device *pdev)
{
	struct resource *res;
	int i, ret = 0;

	for (i = 0; i < ARRAY_SIZE(rt5081_chg_irq_desc); i++) {
		if (!rt5081_chg_irq_desc[i].name)
			continue;
		res = platform_get_resource_byname(pdev, IORESOURCE_IRQ,
					rt5081_chg_irq_desc[i].name);
		if (!res)
			continue;
		ret = devm_request_threaded_irq(&pdev->dev, res->start, NULL,
					rt5081_chg_irq_desc[i].irq_handler,
					IRQF_TRIGGER_FALLING,
					rt5081_chg_irq_desc[i].name,
					platform_get_drvdata(pdev));
		if (ret < 0) {
			dev_err(&pdev->dev, "request %s irq fail\n", res->name);
			continue;
		}
		rt5081_chg_irq_desc[i].irq = res->start;
	}
}

static inline int rt_parse_dt(struct device *dev,
	struct rt5081_pmu_charger_data *chg_data)
{
	struct rt5081_pmu_charger_desc *chg_desc = NULL;
	struct device_node *np = NULL;
	struct i2c_client *i2c = chg_data->chip->i2c;

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);
	chg_data->chg_desc = &rt5081_default_chg_desc;

	np = of_find_node_by_name(NULL, "rt5081_charger");
	if (!np) {
		battery_log(BAT_LOG_CRTI, "%s: find charger node failed",
			__func__);
		return -EINVAL;
	}

	chg_desc = devm_kzalloc(&i2c->dev, sizeof(struct rt5081_pmu_charger_desc),
		GFP_KERNEL);
	if (!chg_desc) {
		battery_log(BAT_LOG_CRTI, "%s: no enough memory\n", __func__);
		return -ENOMEM;
	}

	if (of_property_read_string(np, "charger_name",
		&(chg_data->mchr_info.name)) < 0) {
		battery_log(BAT_LOG_CRTI, "%s: no charger name\n", __func__);
		return -EINVAL;
	}

	if (of_property_read_u32(np, "ichg", &chg_desc->ichg) < 0)
		battery_log(BAT_LOG_CRTI, "%s: no ichg\n", __func__);

	if (of_property_read_u32(np, "aicr", &chg_desc->aicr) < 0)
		battery_log(BAT_LOG_CRTI, "%s: no aicr\n", __func__);

	if (of_property_read_u32(np, "mivr", &chg_desc->mivr) < 0)
		battery_log(BAT_LOG_CRTI, "%s: no mivr\n", __func__);

	if (of_property_read_u32(np, "cv", &chg_desc->cv) < 0)
		battery_log(BAT_LOG_CRTI, "%s: no cv\n", __func__);

	if (of_property_read_u32(np, "ieoc", &chg_desc->ieoc) < 0)
		battery_log(BAT_LOG_CRTI, "%s: no ieoc\n", __func__);

	if (of_property_read_u32(np, "safety_timer",
		&chg_desc->safety_timer) < 0)
		battery_log(BAT_LOG_CRTI, "%s: no safety timer\n", __func__);

	if (of_property_read_u32(np, "dc_wdt", &chg_desc->dc_wdt) < 0)
		battery_log(BAT_LOG_CRTI, "%s: no dc wdt\n", __func__);

	if (of_property_read_u32(np, "ircmp_resistor",
		&chg_desc->ircmp_resistor) < 0)
		battery_log(BAT_LOG_CRTI, "%s: no ircmp resistor\n", __func__);

	if (of_property_read_u32(np, "ircmp_vclamp",
		&chg_desc->ircmp_vclamp) < 0)
		battery_log(BAT_LOG_CRTI, "%s: no ircmp vclamp\n", __func__);

	chg_desc->enable_te = of_property_read_bool(np, "enable_te");
	chg_desc->enable_wdt = of_property_read_bool(np, "enable_wdt");

	chg_data->chg_desc = chg_desc;

	return 0;
}

static int rt5081_pmu_charger_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct rt5081_pmu_charger_data *chg_data;
	bool use_dt = pdev->dev.of_node;

	chg_data = devm_kzalloc(&pdev->dev, sizeof(*chg_data), GFP_KERNEL);
	if (!chg_data)
		return -ENOMEM;
	if (use_dt)
		rt_parse_dt(&pdev->dev, chg_data);

	mutex_init(&chg_data->adc_access_lock);
	mutex_init(&chg_data->irq_access_lock);
	chg_data->chip = dev_get_drvdata(pdev->dev.parent);
	chg_data->dev = &pdev->dev;
	platform_set_drvdata(pdev, chg_data);

	rt5081_pmu_charger_irq_register(pdev);

	rt5081_chg_hw_init(chg_data);
	rt5081_chg_sw_init(chg_data);

	ret = rt5081_chg_sw_workaround(chg_data);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI,
			"%s: software workaround failed\n", __func__);
		goto err_chg_sw_init;
	}

	/* Create single thread workqueue */
	chg_data->discharging_workqueue =
		create_singlethread_workqueue("discharging_workqueue");
	if (!chg_data->discharging_workqueue) {
		ret = -ENOMEM;
		goto err_create_wq;
	}

	/* Init discharging workqueue */
	INIT_WORK(&chg_data->discharging_work,
		rt5081_discharging_work_handler);

	rt_charger_dump_register(&chg_data->mchr_info, NULL);


	chg_data->mchr_info.mchr_intf = rt5081_mchr_intf;
	mtk_charger_set_info(&chg_data->mchr_info);
	battery_charging_control = chr_control_interface;
	chargin_hw_init_done = KAL_TRUE;

	dev_info(&pdev->dev, "%s successfully\n", __func__);
	return ret;

err_chg_sw_init:
err_create_wq:
	mutex_destroy(&chg_data->adc_access_lock);
	mutex_destroy(&chg_data->irq_access_lock);
	return ret;
}

static int rt5081_pmu_charger_remove(struct platform_device *pdev)
{
	struct rt5081_pmu_charger_data *chg_data = platform_get_drvdata(pdev);

	mutex_destroy(&chg_data->adc_access_lock);
	mutex_destroy(&chg_data->irq_access_lock);
	dev_info(chg_data->dev, "%s successfully\n", __func__);
	return 0;
}

static const struct of_device_id rt_ofid_table[] = {
	{ .compatible = "richtek,rt5081_pmu_charger", },
	{ },
};
MODULE_DEVICE_TABLE(of, rt_ofid_table);

static const struct platform_device_id rt_id_table[] = {
	{ "rt5081_pmu_charger", 0},
	{ },
};
MODULE_DEVICE_TABLE(platform, rt_id_table);

static struct platform_driver rt5081_pmu_charger = {
	.driver = {
		.name = "rt5081_pmu_charger",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(rt_ofid_table),
	},
	.probe = rt5081_pmu_charger_probe,
	.remove = rt5081_pmu_charger_remove,
	.id_table = rt_id_table,
};
module_platform_driver(rt5081_pmu_charger);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("cy_huang <cy_huang@richtek.com>");
MODULE_DESCRIPTION("Richtek RT5081 PMU Charger");
MODULE_VERSION("1.0.0_G");
