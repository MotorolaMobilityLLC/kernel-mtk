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
#include <linux/wait.h>
#include <linux/jiffies.h>

#include <mtk_charger_intf.h>
#include <mt-plat/charging.h>
#include <mt-plat/battery_common.h>
#include <mt-plat/mtk_boot_common.h>
#include <mach/mtk_pe.h>

#include "inc/rt5081_pmu_charger_gm20.h"
#include "inc/rt5081_pmu.h"

#define RT5081_PMU_CHARGER_DRV_VERSION	"1.0.6_MTK"

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
	struct work_struct aicl_work;
	struct workqueue_struct *discharging_workqueue;
	wait_queue_head_t wait_queue;
	bool err_state;
	bool to_enable_otg;
	CHARGER_TYPE chr_type;
	u8 chg_irq1;
	u8 chg_irq5;
	u8 chg_irq6;
	u8 dpdm_irq;
};

/* These default values will be used if there's no property in dts */
static struct rt5081_pmu_charger_desc rt5081_default_chg_desc = {
	.ichg = 200000,	/* 10 uA */
	.aicr = 50000,	/* 10 uA */
	.mivr = 4400,
	.cv = 4350000,	/* uA */
	.ieoc = 250,
	.safety_timer = 12,
	.ircmp_resistor = 25,
	.ircmp_vclamp = 32,
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
	0, 125, 250, 500, 1000, 2000, 4000, 8000,
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

static const unsigned char rt5081_reg_en_hidden_mode[] = {
	RT5081_PMU_REG_HIDDENPASCODE1,
	RT5081_PMU_REG_HIDDENPASCODE2,
	RT5081_PMU_REG_HIDDENPASCODE3,
	RT5081_PMU_REG_HIDDENPASCODE4,
};

static const unsigned char rt5081_val_en_hidden_mode[] = {
	0x96, 0x69, 0xC3, 0x3C,
};

enum rt5081_iin_limit_sel {
	RT5081_IIMLMTSEL_AICR_3250 = 0,
	RT5081_IIMLMTSEL_CHR_TYPE,
	RT5081_IINLMTSEL_AICR,
	RT5081_IINLMTSEL_LOWER_LEVEL, /* lower of above three */
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
	RT5081_PMU_REG_CHGSTAT1,
	RT5081_PMU_REG_CHGSTAT2,
	RT5081_PMU_REG_CHGSTAT3,
	RT5081_PMU_REG_CHGSTAT4,
	RT5081_PMU_REG_CHGSTAT5,
	RT5081_PMU_REG_CHGSTAT6,
	RT5081_PMU_REG_QCSTAT,
	RT5081_PMU_REG_DICHGSTAT,
	RT5081_PMU_REG_OVPCTRLSTAT,
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
static int rt_charger_enable_hz(struct mtk_charger_info *mchr_info, void *data);
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

static inline void rt5081_chg_irq_set_flag(
	struct rt5081_pmu_charger_data *chg_data, u8 *irq, u8 mask)
{
	mutex_lock(&chg_data->irq_access_lock);
	*irq |= mask;
	mutex_unlock(&chg_data->irq_access_lock);
}

static inline void rt5081_chg_irq_clr_flag(
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
		if (target_value >= value_table[i] &&
		    target_value < value_table[i + 1])
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

	pr_info("%s: timer = %d\n", __func__, hour);

	reg_fct = rt5081_find_closest_reg_value(
		RT5081_WT_FC_MIN,
		RT5081_WT_FC_MAX,
		RT5081_WT_FC_STEP,
		RT5081_WT_FC_NUM,
		hour
	);

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

	pr_info("%s: enable = %d\n", __func__, enable);

	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGCTRL3, RT5081_MASK_ILIM_EN);

	return ret;
}

/* Select IINLMTSEL to use AICR */
static int rt5081_select_input_current_limit(
	struct rt5081_pmu_charger_data *chg_data, enum rt5081_iin_limit_sel sel)
{
	int ret = 0;

	pr_info("%s: select input current limit = %d\n", __func__, sel);

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

	pr_info("%s: enable = %d\n", __func__, enable);

	/* Disable hidden mode */
	if (!enable) {
		ret = rt5081_pmu_reg_write(chg_data->chip,
			rt5081_reg_en_hidden_mode[0], 0x00);
		if (ret < 0)
			goto err;
		return ret;
	}

	ret = rt5081_pmu_reg_block_write(
		chg_data->chip,
		rt5081_reg_en_hidden_mode[0],
		ARRAY_SIZE(rt5081_val_en_hidden_mode),
		rt5081_val_en_hidden_mode
	);
	if (ret < 0)
		goto err;

	return ret;

err:
	pr_err("%s: enable = %d failed, ret = %d\n", __func__, enable, ret);
	return ret;
}

static int rt5081_set_vrechg(struct rt5081_pmu_charger_data *chg_data,
	u32 vrechg)
{
	int ret = 0;
	u8 reg_vrechg = 0;

	/* Find corresponding reg value */
	reg_vrechg = rt5081_find_closest_reg_value(
		RT5081_VRECHG_MIN,
		RT5081_VRECHG_MAX,
		RT5081_VRECHG_STEP,
		RT5081_VRECHG_NUM,
		vrechg
	);

	pr_info("%s: vrechg = %d (0x%02X)\n", __func__, vrechg, reg_vrechg);

	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGCTRL11,
		RT5081_MASK_VRECHG,
		reg_vrechg << RT5081_SHIFT_VRECHG
	);

	return ret;

}

#if 0
static int rt5081_set_iprec(struct rt5081_pmu_charger_data *chg_data,
	u32 iprec)
{
	int ret = 0;
	u8 reg_iprec = 0;

	/* Find corresponding reg value */
	reg_iprec = rt5081_find_closest_reg_value(RT5081_IPREC_MIN,
		RT5081_IPREC_MAX, RT5081_IPREC_STEP, RT5081_IPREC_NUM, iprec);

	battery_log(BAT_LOG_CRTI, "%s: iprec = %d\n", __func__, iprec);

	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGCTRL8,
		RT5081_MASK_IPREC,
		reg_iprec << RT5081_SHIFT_IPREC
	);

	return ret;
}
#endif

/* Software workaround */
static int rt5081_chg_sw_workaround(struct rt5081_pmu_charger_data *chg_data)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	/* Enter hidden mode */
	ret = rt5081_enable_hidden_mode(chg_data, true);
	if (ret < 0)
		goto out;

	/* Disable TS auto sensing */
	ret = rt5081_pmu_reg_clr_bit(chg_data->chip,
		RT5081_PMU_REG_CHGHIDDENCTRL15, 0x01);

	/* Disable SEN_DCP for charging mode */
	ret = rt5081_pmu_reg_clr_bit(chg_data->chip,
		RT5081_PMU_REG_QCCTRL2, RT5081_MASK_EN_DCP);

out:
	/* Exit hidden mode */
	ret = rt5081_enable_hidden_mode(chg_data, false);
	if (ret < 0)
		pr_err("%s: exit hidden mode failed\n", __func__);

	return ret;
}

static int rt5081_get_adc(struct rt5081_pmu_charger_data *chg_data,
	enum rt5081_adc_sel adc_sel, int *adc_val)
{
	int ret = 0;
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
		pr_err("%s: select ch to %d failed, ret = %d\n",
			__func__, adc_sel, ret);
		goto out;
	}

	/* Clear adc done event */
	rt5081_chg_irq_clr_flag(chg_data, &chg_data->chg_irq6,
		RT5081_MASK_ADC_DONEI);

	/* Start ADC conversation */
	ret = rt5081_pmu_reg_set_bit(chg_data->chip, RT5081_PMU_REG_CHGADC,
		RT5081_MASK_ADC_START);
	if (ret < 0) {
		pr_err("%s: start conversation failed, sel = %d, ret = %d\n",
			__func__, adc_sel, ret);
		goto out;
	}

	/* Wait for ADC conversation */
	ret = wait_event_interruptible_timeout(chg_data->wait_queue,
		chg_data->chg_irq6 & RT5081_MASK_ADC_DONEI,
		msecs_to_jiffies(150));
	if (ret <= 0) {
		pr_err("%s: wait conversation failed, sel = %d, ret = %d\n",
			__func__, adc_sel, ret);
		ret = -EINVAL;
		goto out;
	}

	pr_debug("%s: sel = %d\n", __func__, adc_sel);

	mdelay(1);

	/* Read ADC data */
	ret = rt5081_pmu_reg_block_read(chg_data->chip, RT5081_PMU_REG_ADCDATAH,
		2, adc_data);
	if (ret < 0) {
		pr_err("%s: read ADC data failed, ret = %d\n", __func__, ret);
		goto out;
	}

	pr_debug("%s: adc_sel = %d, adc_h = 0x%02X, adc_l = 0x%02X\n",
		__func__, adc_sel, adc_data[0], adc_data[1]);

	/* Calculate ADC value */
	*adc_val = (adc_data[0] * 256 + adc_data[1]) * rt5081_adc_unit[adc_sel]
		+ rt5081_adc_offset[adc_sel];

out:
	mutex_unlock(&chg_data->adc_access_lock);
	return ret;
}


static int rt5081_enable_watchdog_timer(
	struct rt5081_pmu_charger_data *chg_data, bool enable)
{
	int ret = 0;

	pr_info("%s: enable = %d\n", __func__, enable);
	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGCTRL13, RT5081_MASK_WDT_EN);

	return ret;
}

static int rt5081_is_charging_enable(struct rt5081_pmu_charger_data *chg_data,
	bool *enable)
{
	int ret = 0;

	ret = rt5081_pmu_reg_test_bit(chg_data->chip, RT5081_PMU_REG_CHGCTRL2,
		RT5081_SHIFT_CHG_EN);
	if (ret < 0) {
		*enable = false;
		return ret;
	}

	*enable = (ret == 0 ? false : true);

	return ret;
}

static int rt5081_enable_te(struct rt5081_pmu_charger_data *chg_data,
	bool enable)
{
	int ret = 0;

	pr_info("%s: enable = %d\n", __func__, enable);
	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGCTRL2, RT5081_MASK_TE_EN);

	return ret;
}

static int rt5081_enable_pump_express(struct rt5081_pmu_charger_data *chg_data,
	bool enable)
{
	int ret = 0;

	pr_info("%s: enable %d\n", __func__, enable);

	/* clear pumpx done event */
	rt5081_chg_irq_clr_flag(chg_data, &chg_data->chg_irq6,
		RT5081_MASK_PUMPX_DONEI);

	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGCTRL17, RT5081_MASK_PUMPX_EN);
	if (ret < 0)
		return ret;

	ret = wait_event_interruptible_timeout(chg_data->wait_queue,
		chg_data->chg_irq6 & RT5081_MASK_PUMPX_DONEI,
		msecs_to_jiffies(7500));
	if (ret <= 0) {
		pr_err("%s: wait failed, ret = %d\n", __func__, ret);
		ret = -EIO;
		return ret;
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
	*ieoc = rt5081_find_closest_real_value(
		RT5081_IEOC_MIN,
		RT5081_IEOC_MAX,
		RT5081_IEOC_STEP,
		reg_ieoc
	);

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
	*mivr = rt5081_find_closest_real_value(
		RT5081_MIVR_MIN,
		RT5081_MIVR_MAX,
		RT5081_MIVR_STEP,
		reg_mivr
	);

	return ret;
}

static int rt5081_set_ieoc(struct rt5081_pmu_charger_data *chg_data,
	const u32 ieoc)
{
	int ret = 0;

	/* Find corresponding reg value */
	u8 reg_ieoc = rt5081_find_closest_reg_value(
		RT5081_IEOC_MIN,
		RT5081_IEOC_MAX,
		RT5081_IEOC_STEP,
		RT5081_IEOC_NUM,
		ieoc
	);

	pr_info("%s: ieoc = %d (0x%02X)\n", __func__, ieoc, reg_ieoc);

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

	pr_info("%s: timer = %dms(0x%02X)\n", __func__, ms, reg_wdt);

	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGDIRCHG2,
		RT5081_MASK_DC_WDT,
		reg_wdt << RT5081_SHIFT_DC_WDT
	);

	return ret;
}

static int rt5081_enable_jeita(struct rt5081_pmu_charger_data *chg_data,
	bool enable)
{
	int ret = 0;

	pr_info("%s: enable = %d\n", __func__, enable);

	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGCTRL16, RT5081_MASK_JEITA_EN);

	return ret;
}

static int rt5081_set_aicl_vth(struct rt5081_pmu_charger_data *chg_data,
	u32 aicl_vth)
{
	int ret = 0;
	u8 reg_aicl_vth = 0;

	reg_aicl_vth = rt5081_find_closest_reg_value(
		RT5081_AICL_VTH_MIN,
		RT5081_AICL_VTH_MAX,
		RT5081_AICL_VTH_STEP,
		RT5081_AICL_VTH_NUM,
		aicl_vth
	);

	pr_info("%s: vth = %d (0x%02X)\n", __func__, aicl_vth, reg_aicl_vth);

	ret = rt5081_pmu_reg_update_bits(
		chg_data->chip,
		RT5081_PMU_REG_CHGCTRL14,
		RT5081_MASK_AICL_VTH,
		reg_aicl_vth << RT5081_SHIFT_AICL_VTH
	);

	if (ret < 0)
		pr_err("%s: set aicl vth failed, ret = %d\n", __func__, ret);

	return ret;
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
	*voreg = rt5081_find_closest_real_value(
		RT5081_BAT_VOREG_MIN,
		RT5081_BAT_VOREG_MAX,
		RT5081_BAT_VOREG_STEP,
		reg_voreg
	);

	return ret;
}

static int rt5081_enable_usb_chrdet(struct rt5081_pmu_charger_data *chg_data,
	bool enable)
{
	int ret = 0;

	pr_info("%s: enable = %d\n", __func__, enable);

	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_DEVICETYPE, RT5081_MASK_USBCHGEN);

	return ret;

}

static int rt5081_chg_init_setting(struct rt5081_pmu_charger_data *chg_data)
{
	int ret = 0;
	struct rt5081_pmu_charger_desc *chg_desc = chg_data->chg_desc;
	bool enable_safety_timer = true;
	bool enable_hz = false;
#ifdef CONFIG_MTK_BIF_SUPPORT
	u32 ircmp_resistor = 0;
	u32 ircmp_vclamp = 0;
#endif

	pr_info("%s\n", __func__);

	/* Disable hardware ILIM */
	ret = rt5081_enable_ilim(chg_data, false);
	if (ret < 0)
		pr_err("%s: disable ilim failed\n", __func__);

	/* Select IINLMTSEL to use AICR */
	ret = rt5081_select_input_current_limit(chg_data,
		RT5081_IINLMTSEL_AICR);
	if (ret < 0)
		pr_err("%s: select iinlmtsel failed\n", __func__);

	ret = rt_charger_set_ichg(&chg_data->mchr_info, &chg_desc->ichg);
	if (ret < 0)
		pr_err("%s: set ichg failed\n", __func__);

	ret = rt_charger_set_aicr(&chg_data->mchr_info, &chg_desc->aicr);
	if (ret < 0)
		pr_err("%s: set aicr failed\n", __func__);

	ret = rt_charger_set_mivr(&chg_data->mchr_info, &chg_desc->mivr);
	if (ret < 0)
		pr_err("%s: set mivr failed\n", __func__);

	ret = rt_charger_set_battery_voreg(&chg_data->mchr_info, &chg_desc->cv);
	if (ret < 0)
		pr_err("%s: set voreg failed\n", __func__);

	ret = rt5081_set_ieoc(chg_data, chg_desc->ieoc); /* mA */
	if (ret < 0)
		pr_err("%s: set ieoc failed\n", __func__);

	/* Enable TE */
	ret = rt5081_enable_te(chg_data, chg_desc->enable_te);
	if (ret < 0)
		pr_err("%s: set te failed\n", __func__);

	/* Set fast charge timer */
	ret = rt5081_set_fast_charge_timer(chg_data, chg_desc->safety_timer);
	if (ret < 0)
		pr_err("%s: set fast timer failed\n", __func__);

	/* Set DC watch dog timer */
	ret = rt5081_set_dc_watchdog_timer(chg_data, chg_desc->dc_wdt);
	if (ret < 0)
		pr_err("%s: set dc watch dog timer failed\n", __func__);

	/* Enable charger timer */
	ret = rt_charger_enable_safety_timer(&chg_data->mchr_info,
		&enable_safety_timer);
	if (ret < 0)
		pr_err("%s: enable charger timer failed\n", __func__);

	/* Enable watchdog timer */
	ret = rt5081_enable_watchdog_timer(chg_data, true);
	if (ret < 0)
		pr_err("%s: enable watchdog timer failed\n", __func__);

	/* Disable JEITA */
	ret = rt5081_enable_jeita(chg_data, false);
	if (ret < 0)
		pr_err("%s: disable jeita failed\n", __func__);

	ret = rt_charger_enable_hz(&chg_data->mchr_info, &enable_hz);
	if (ret < 0)
		pr_err("%s: disable hz failed\n", __func__);

	ret = rt5081_set_vrechg(chg_data, 100);
	if (ret < 0)
		pr_err("%s: set vrechg failed\n", __func__);

#ifdef CONFIG_TCPC_CLASS
	/* Disable USB charger type detection */
	ret = rt5081_enable_usb_chrdet(chg_data, false);
#else
	/* Enable USB charger type detection */
	ret = rt5081_enable_usb_chrdet(chg_data, true);
#endif
	if (ret < 0)
		pr_err("%s: set usb chrdet failed\n", __func__);

	/* Set ircomp according to BIF */
#ifdef CONFIG_MTK_BIF_SUPPORT
	ret = rt_charger_set_ircmp_resistor(&chg_data->mchr_info,
		&ircmp_resistor);
	if (ret < 0)
		pr_err("%s: set IR compensation resistor failed\n", __func__);

	ret = rt_charger_set_ircmp_vclamp(&chg_data->mchr_info, &ircmp_vclamp);
	if (ret < 0)
		pr_err("%s: set IR compensation vclamp failed\n", __func__);
#else
	ret = rt_charger_set_ircmp_resistor(&chg_data->mchr_info,
		&chg_desc->ircmp_resistor);
	if (ret < 0)
		pr_err("%s: set IR compensation resistor failed\n", __func__);

	ret = rt_charger_set_ircmp_vclamp(&chg_data->mchr_info,
		&chg_desc->ircmp_vclamp);
	if (ret < 0)
		pr_err("%s: set IR compensation vclamp failed\n", __func__);
#endif

	return ret;
}

static void rt5081_discharging_work_handler(struct work_struct *work)
{
	int ret = 0, i = 0;
	const unsigned int max_retry_cnt = 3;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)container_of(work,
		struct rt5081_pmu_charger_data, discharging_work);

	ret = rt5081_enable_hidden_mode(chg_data, true);
	if (ret < 0)
		return;

	pr_info("%s\n", __func__);

	/* Set bit2 of reg[0x31] to 1 to enable discharging */
	ret = rt5081_pmu_reg_set_bit(chg_data->chip,
		RT5081_PMU_REG_CHGHIDDENCTRL1, 0x04);
	if (ret < 0) {
		pr_err("%s: enable discharging failed\n", __func__);
		goto out;
	}

	/* Wait for discharging to 0V */
	mdelay(20);

	for (i = 0; i < max_retry_cnt; i++) {
		/* Disable discharging */
		ret = rt5081_pmu_reg_clr_bit(chg_data->chip,
			RT5081_PMU_REG_CHGHIDDENCTRL1, 0x04);
		if (ret < 0)
			continue;
		if (rt5081_pmu_reg_test_bit(chg_data->chip,
			RT5081_PMU_REG_CHGHIDDENCTRL1, 2) == 0)
			goto out;
	}

	pr_err("%s: disable discharging failed\n", __func__);
out:
	rt5081_enable_hidden_mode(chg_data, false);
	pr_info("%s: end\n", __func__);
}

static int rt5081_run_discharging(struct rt5081_pmu_charger_data *chg_data)
{
	if (!queue_work(chg_data->discharging_workqueue,
		&chg_data->discharging_work))
		return -EINVAL;

	return 0;
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
		for (i = 0; i < ARRAY_SIZE(rt5081_chg_reg_addr); i++) {
			ret = rt5081_pmu_reg_read(chg_data->chip,
				rt5081_chg_reg_addr[i]);
			if (ret < 0)
				return ret;

			pr_err("%s: reg[0x%02X] = 0x%02X\n",
				__func__, rt5081_chg_reg_addr[i], ret);
		}
	}

	pr_info("%s: ICHG = %dmA, AICR = %dmA, MIVR = %dmV, IEOC = %dmA\n",
		__func__, ichg / 100, aicr / 100, mivr, ieoc);

	pr_info("%s: VSYS = %dmV, VBAT = %dmV IBAT = %dmA, IBUS = %dmA\n",
		__func__, adc_vsys, adc_vbat, adc_ibat, adc_ibus);

	pr_info("%s: CHG_EN = %d, CHG_STATUS = %s\n",
		__func__, chg_enable, rt5081_chg_status_name[chg_status]);

	ret = 0;
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

	pr_info("%s: enable = %d\n", __func__, enable);

	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGCTRL1, RT5081_MASK_HZ_EN);

	return ret;
}

static int rt_charger_enable_safety_timer(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u8 enable = *((u8 *)data);
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	pr_info("%s: enable = %d\n", __func__, enable);

	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGCTRL12, RT5081_MASK_TMR_EN);

	return ret;
}

static int rt_charger_enable_otg(struct mtk_charger_info *mchr_info, void *data)
{
	u8 enable = *((u8 *)data);
	int ret = 0;
	u32 current_limit = 500; /* mA */
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	pr_info("%s: enable = %d\n", __func__, enable);

#ifndef CONFIG_TCPC_CLASS
	/* Turn off USB charger detection */
	if (enable) {
		ret = rt5081_enable_usb_chrdet(chg_data, !enable);
		if (ret < 0)
			pr_err("%s: disable usb chrdet failed\n", __func__);
	}
#endif

	/* Set OTG_OC to 500mA */
	ret = rt_charger_set_boost_current_limit(mchr_info, &current_limit);
	if (ret < 0)
		return ret;

	/* Clear soft start event */
	rt5081_chg_irq_clr_flag(chg_data, &chg_data->chg_irq5,
		RT5081_MASK_CHG_SSFINISHI);

	/* Switch OPA mode to boost mode */
	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_CHGCTRL1, RT5081_MASK_OPA_MODE);

	/* Enable/disable SEN_DCP */
	ret = (enable ? rt5081_pmu_reg_set_bit : rt5081_pmu_reg_clr_bit)
		(chg_data->chip, RT5081_PMU_REG_QCCTRL2, RT5081_MASK_EN_DCP);

	if (!enable) {
		/* Enter hidden mode */
		ret = rt5081_enable_hidden_mode(chg_data, true);
		if (ret < 0)
			return ret;

		/* Workaround: reg[0x25] = 0x0F after leaving OTG mode */
		ret = rt5081_pmu_reg_write(chg_data->chip,
			RT5081_PMU_REG_CHGHIDDENCTRL6, 0x0F);
		if (ret < 0)
			pr_err("%s: otg workaroud failed\n", __func__);

		/* Exit hidden mode */
		ret = rt5081_enable_hidden_mode(chg_data, false);

		ret = rt5081_run_discharging(chg_data);
		if (ret < 0)
			pr_err("%s: run discharging failed\n", __func__);

		return ret;
	}

	/* Wait for soft start finish interrupt */
	ret = wait_event_interruptible_timeout(chg_data->wait_queue,
		chg_data->chg_irq5 & RT5081_MASK_CHG_SSFINISHI,
		msecs_to_jiffies(50));

	if (ret <= 0) {
		pr_err("%s: otg soft start failed, ret = %d\n", __func__, ret);
		/* Disable OTG */
		rt5081_pmu_reg_clr_bit(chg_data->chip, RT5081_PMU_REG_CHGCTRL1,
				RT5081_MASK_OPA_MODE);
		ret = -EIO;
		return ret;
	}


	/* Enter hidden mode */
	ret = rt5081_enable_hidden_mode(chg_data, true);
	if (ret < 0)
		return ret;

	/* Woraround reg[0x25] = 0x00 after entering OTG mode */
	ret = rt5081_pmu_reg_write(chg_data->chip,
			RT5081_PMU_REG_CHGHIDDENCTRL6, 0x00);
	if (ret < 0)
		pr_err("%s: otg workaroud failed\n", __func__);

	/* Exit hidden mode */
	ret = rt5081_enable_hidden_mode(chg_data, false);
	pr_info("%s: otg soft start successfully\n", __func__);

	return ret;
}

static int rt_charger_enable_power_path(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	u8 enable = *((u8 *)data);
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	pr_info("%s: enable = %d\n", __func__, enable);

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

	pr_info("%s: enable = %d\n", __func__, enable);

	if (enable) {
		/* Enable bypass mode */
		ret = rt5081_pmu_reg_set_bit(chg_data->chip,
			RT5081_PMU_REG_CHGCTRL2, RT5081_MASK_BYPASS_MODE);
		if (ret < 0) {
			pr_err("%s: enable bypass mode failed\n", __func__);
			return ret;
		}

		/* VG_EN = 1 */
		ret = rt5081_pmu_reg_set_bit(chg_data->chip,
			RT5081_PMU_REG_CHGPUMP, RT5081_MASK_VG_EN);
		if (ret < 0)
			pr_err("%s: enable VG_EN failed\n", __func__);

		return ret;
	}

	/* Disable direct charge */
	/* VG_EN = 0 */
	ret = rt5081_pmu_reg_clr_bit(chg_data->chip, RT5081_PMU_REG_CHGPUMP,
		RT5081_MASK_VG_EN);
	if (ret < 0)
		pr_err("%s: disable VG_EN failed\n", __func__);

	/* Disable bypass mode */
	ret = rt5081_pmu_reg_clr_bit(chg_data->chip, RT5081_PMU_REG_CHGCTRL2,
		RT5081_MASK_BYPASS_MODE);
	if (ret < 0)
		pr_err("%s: disable bypass mode failed\n", __func__);

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
	reg_ichg = rt5081_find_closest_reg_value(
		RT5081_ICHG_MIN,
		RT5081_ICHG_MAX,
		RT5081_ICHG_STEP,
		RT5081_ICHG_NUM,
		ichg
	);

	pr_info("%s: ichg = %d (0x%02X)\n", __func__, ichg, reg_ichg);

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
	reg_aicr = rt5081_find_closest_reg_value(
		RT5081_AICR_MIN,
		RT5081_AICR_MAX,
		RT5081_AICR_STEP,
		RT5081_AICR_NUM,
		aicr
	);

	pr_info("%s: aicr = %d (0x%02X)\n", __func__, aicr, reg_aicr);

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
	reg_mivr = rt5081_find_closest_reg_value(
		RT5081_MIVR_MIN,
		RT5081_MIVR_MAX,
		RT5081_MIVR_STEP,
		RT5081_MIVR_NUM,
		mivr
	);

	pr_info("%s: mivr = %d (0x%02X)\n", __func__, mivr, reg_mivr);

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

	reg_voreg = rt5081_find_closest_reg_value(
		RT5081_BAT_VOREG_MIN,
		RT5081_BAT_VOREG_MAX,
		RT5081_BAT_VOREG_STEP,
		RT5081_BAT_VOREG_NUM,
		voreg
	);

	pr_info("%s: bat voreg = %d (0x%02X)\n", __func__, voreg, reg_voreg);

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

	pr_info("%s: ilimit = %d (0x%02X)\n", __func__, current_limit,
		reg_ilimit);

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

	pr_info("%s: pump_up = %d\n", __func__, is_pump_up);

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

	pr_info("%s\n", __func__);
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

	pr_info("%s: VTA = %d\n", __func__, voltage);

	/* Set to PEP2.0 */
	ret = rt5081_pmu_reg_set_bit(chg_data->chip, RT5081_PMU_REG_CHGCTRL17,
		RT5081_MASK_PUMPX_20_10);
	if (ret < 0)
		return ret;

	/* Find register value of target voltage */
	reg_volt = rt5081_find_closest_reg_value(
		RT5081_PEP20_VOLT_MIN,
		RT5081_PEP20_VOLT_MAX,
		RT5081_PEP20_VOLT_STEP,
		RT5081_PEP20_VOLT_NUM,
		voltage
	);

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
	ichg = rt5081_find_closest_real_value(
		RT5081_ICHG_MIN,
		RT5081_ICHG_MAX,
		RT5081_ICHG_STEP,
		reg_ichg
	);

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
	aicr = rt5081_find_closest_real_value(
		RT5081_AICR_MIN,
		RT5081_AICR_MAX,
		RT5081_AICR_STEP,
		reg_aicr
	);

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

	pr_info("%s: temperature = %d\n", __func__, adc_temp);

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

	pr_debug("%s: ibus = %dmA\n", __func__, adc_ibus);
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

	pr_debug("%s: vbus = %dmA\n", __func__, adc_vbus);
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
	int ret = 0;
	u32 mivr = 0, aicl_vth = 0, aicr = 0;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	/* Check whether MIVR loop is active */
	if (!(chg_data->chg_irq1 & RT5081_MASK_CHG_MIVR)) {
		pr_info("%s: mivr loop is not active\n", __func__);
		return 0;
	}
	pr_info("%s: mivr loop is active\n", __func__);

	/* Clear chg mivr event */
	rt5081_chg_irq_clr_flag(chg_data, &chg_data->chg_irq1,
		RT5081_MASK_CHG_MIVR);

	ret = rt5081_get_mivr(chg_data, &mivr);
	if (ret < 0)
		goto out;

	/* Check if there's a suitable AICL_VTH */
	aicl_vth = mivr + 200;
	if (aicl_vth > RT5081_AICL_VTH_MAX) {
		pr_info("%s: no suitable VTH, vth = %d\n", __func__, aicl_vth);
		ret = -EINVAL;
		goto out;
	}

	ret = rt5081_set_aicl_vth(chg_data, aicl_vth);
	if (ret < 0)
		goto out;

	/* Clear AICL measurement IRQ */
	rt5081_chg_irq_clr_flag(chg_data, &chg_data->chg_irq5,
		RT5081_MASK_CHG_AICLMEASI);

	ret = rt5081_pmu_reg_set_bit(chg_data->chip, RT5081_PMU_REG_CHGCTRL14,
		RT5081_MASK_AICL_MEAS);
	if (ret < 0)
		goto out;

	ret = wait_event_interruptible_timeout(chg_data->wait_queue,
		chg_data->chg_irq5 & RT5081_MASK_CHG_AICLMEASI,
		msecs_to_jiffies(2500));
	if (ret <= 0) {
		pr_err("%s: wait AICL time out, ret = %d\n", __func__, ret);
		ret = -EIO;
		goto out;
	}

	ret = rt_charger_get_aicr(mchr_info, &aicr);
	if (ret < 0)
		goto out;

	*((u32 *)data) = aicr / 100;
	pr_info("%s: aicr upper bound = %dmA\n", __func__, aicr / 100);

	return 0;

out:
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

	reg_vbusov = rt5081_find_closest_reg_value(
		RT5081_DC_VBUSOV_LVL_MIN,
		RT5081_DC_VBUSOV_LVL_MAX,
		RT5081_DC_VBUSOV_LVL_STEP,
		RT5081_DC_VBUSOV_LVL_NUM,
		vbusov
	);

	pr_info("%s: vbusov = %dmV (0x%02X)\n", __func__, vbusov, reg_vbusov);

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

	pr_info("%s: enable = %d\n", __func__, enable);
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

	reg_ibusoc = rt5081_find_closest_reg_value(
		RT5081_DC_IBUSOC_LVL_MIN,
		RT5081_DC_IBUSOC_LVL_MAX,
		RT5081_DC_IBUSOC_LVL_STEP,
		RT5081_DC_IBUSOC_LVL_NUM,
		ibusoc
	);

	pr_info("%s: ibusoc = %dmA (0x%02X)\n", __func__, ibusoc, reg_ibusoc);

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

	pr_info("%s: enable = %d\n", __func__, enable);
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



	ret = rt5081_get_battery_voreg(chg_data, &voreg);
	if (ret < 0) {
		pr_err("%s: get voreg failed\n", __func__);
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

	pr_info("%s: vbatov = %dmV (0x%02X)\n", __func__, vbatov, reg_vbatov);

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
	u32 resistor = *((u32 *)data);
	u8 reg_resistor = 0;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	reg_resistor = rt5081_find_closest_reg_value(
		RT5081_IRCMP_RES_MIN,
		RT5081_IRCMP_RES_MAX,
		RT5081_IRCMP_RES_STEP,
		RT5081_IRCMP_RES_NUM,
		resistor
	);

	pr_info("%s: resistor = %d (0x%02X)\n", __func__, resistor,
		reg_resistor);

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
	u32 vclamp = *((u32 *)data);
	u8 reg_vclamp = 0;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	reg_vclamp = rt5081_find_closest_reg_value(
		RT5081_IRCMP_VCLAMP_MIN,
		RT5081_IRCMP_VCLAMP_MAX,
		RT5081_IRCMP_VCLAMP_STEP,
		RT5081_IRCMP_VCLAMP_NUM,
		vclamp
	);

	pr_info("%s: vclamp = %d (0x%02X)\n", __func__, vclamp, reg_vclamp);

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

static int rt_charger_get_charger_type(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	pr_info("%s\n", __func__);


#ifndef CONFIG_TCPC_CLASS
	Charger_Detect_Init();

	/* Clear adc done event */
	rt5081_chg_irq_clr_flag(chg_data, &chg_data->dpdm_irq,
		RT5081_MASK_ATTACHI);

	/* Turn off/on USB charger detection to retrigger bc1.2 */
	ret = rt5081_enable_usb_chrdet(chg_data, false);
	if (ret < 0)
		pr_err("%s: disable usb chrdet failed\n", __func__);

	ret = rt5081_enable_usb_chrdet(chg_data, true);
	if (ret < 0)
		pr_err("%s: disable usb chrdet failed\n", __func__);
#endif

	ret = wait_event_interruptible_timeout(chg_data->wait_queue,
		chg_data->dpdm_irq & RT5081_MASK_ATTACHI,
		msecs_to_jiffies(1000));
	if (ret <= 0) {
		pr_err("%s: wait attachi failed, ret = %d\n", __func__, ret);
		chg_data->chr_type = CHARGER_UNKNOWN;
	}

	*(CHARGER_TYPE *)data = chg_data->chr_type;
	pr_info("%s: chr_type = %d\n", __func__, chg_data->chr_type);
	return ret;
}

static int rt_charger_enable_chr_type_det(struct mtk_charger_info *mchr_info,
	void *data)
{
	int ret = 0;

#ifdef CONFIG_TCPC_CLASS
	u8 enable = *(u8 *)data;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)mchr_info;

	pr_info("%s: enable = %d\n", __func__, enable);

	if (enable)
		Charger_Detect_Init();

	/* Clear adc done event */
	rt5081_chg_irq_clr_flag(chg_data, &chg_data->dpdm_irq,
		RT5081_MASK_ATTACHI);

	ret = rt5081_enable_usb_chrdet(chg_data, enable);
	if (ret < 0)
		pr_err("%s: enable = %d, failed\n", __func__, enable);
#endif

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
	[CHARGING_CMD_GET_CHARGER_TYPE] = rt_charger_get_charger_type,
	[CHARGING_CMD_ENABLE_CHR_TYPE_DET] = rt_charger_enable_chr_type_det,

	/*
	 * The following interfaces are not related to charger
	 * Define in mtk_charger_intf.c
	 */
	[CHARGING_CMD_SW_INIT] = mtk_charger_sw_init,
	[CHARGING_CMD_SET_HV_THRESHOLD] = mtk_charger_set_hv_threshold,
	[CHARGING_CMD_GET_HV_STATUS] = mtk_charger_get_hv_status,
	[CHARGING_CMD_GET_BATTERY_STATUS] = mtk_charger_get_battery_status,
	[CHARGING_CMD_GET_CHARGER_DET_STATUS] = mtk_charger_get_charger_det_status,
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
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_aicr_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_mivr_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	rt5081_chg_irq_set_flag(chg_data, &chg_data->chg_irq1,
		RT5081_MASK_CHG_MIVR);

	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_pwr_rdy_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_vinovp_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_vsysuv_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_vsysov_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_vbatov_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_vbusov_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ts_bat_cold_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ts_bat_cool_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ts_bat_warm_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ts_bat_hot_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_tmri_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_batabsi_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_adpbadi_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_rvpi_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_otpi_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_aiclmeasi_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	rt5081_chg_irq_set_flag(chg_data, &chg_data->chg_irq5,
		RT5081_MASK_CHG_AICLMEASI);

	wake_up_interruptible(&chg_data->wait_queue);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_ichgmeasi_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chgdet_donei_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_wdtmri_irq_handler(int irq, void *data)
{
	int ret = 0;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	ret = rt_charger_reset_watchdog_timer(&chg_data->mchr_info, NULL);
	if (ret < 0)
		pr_err("%s: kick wdt failed\n", __func__);

	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ssfinishi_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	rt5081_chg_irq_set_flag(chg_data, &chg_data->chg_irq5,
		RT5081_MASK_CHG_SSFINISHI);

	wake_up_interruptible(&chg_data->wait_queue);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_rechgi_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_termi_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chg_ieoci_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_adc_donei_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	rt5081_chg_irq_set_flag(chg_data, &chg_data->chg_irq6,
		RT5081_MASK_ADC_DONEI);

	wake_up_interruptible(&chg_data->wait_queue);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_pumpx_donei_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	rt5081_chg_irq_set_flag(chg_data, &chg_data->chg_irq6,
		RT5081_MASK_PUMPX_DONEI);
	wake_up_interruptible(&chg_data->wait_queue);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_bst_batuvi_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_bst_vbusovi_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_bst_olpi_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_attachi_irq_handler(int irq, void *data)
{
	int ret = 0;

	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	pr_info("%s\n", __func__);
	Charger_Detect_Release();

	ret = rt5081_pmu_reg_read(chg_data->chip, RT5081_PMU_REG_DEVICETYPE);
	if (ret < 0) {
		pr_err("%s: read charger type failed\n", __func__);
		return IRQ_HANDLED;
	}

	if (ret & RT5081_MASK_DCPSTD)
		chg_data->chr_type = STANDARD_CHARGER;
	else if (ret & RT5081_MASK_CDP)
		chg_data->chr_type = CHARGING_HOST;
	else if (ret & RT5081_MASK_SDP)
		chg_data->chr_type = STANDARD_HOST;
	else
		chg_data->chr_type = CHARGER_UNKNOWN;

	rt5081_chg_irq_set_flag(chg_data, &chg_data->dpdm_irq,
		RT5081_MASK_ATTACHI);

	wake_up_interruptible(&chg_data->wait_queue);

	/* Turn off USB charger detection */
	ret = rt5081_enable_usb_chrdet(chg_data, false);
	if (ret < 0)
		pr_err("%s: disable usb chrdet failed\n", __func__);

	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_detachi_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_qc30stpdone_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_qc_vbusdet_done_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_hvdcp_det_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_chgdeti_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_dcdti_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_dirchg_vgoki_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_dirchg_wdtmri_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_dirchg_uci_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_dirchg_oci_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_dirchg_ovi_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ovpctrl_swon_evt_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ovpctrl_uvp_d_evt_irq_handler(int irq, void *data)
{
#ifndef CONFIG_TCPC_CLASS
	int ret = 0;
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	/* Turn on USB charger detection */
	ret = rt5081_enable_usb_chrdet(chg_data, true);
	if (ret < 0)
		pr_info("%s: enable usb chrdet failed\n", __func__);

	dev_err(chg_data->dev, "%s\n", __func__);
#endif
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ovpctrl_uvp_evt_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ovpctrl_ovp_d_evt_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_ovpctrl_ovp_evt_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_charger_data *chg_data =
		(struct rt5081_pmu_charger_data *)data;

	dev_err(chg_data->dev, "%s\n", __func__);
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
	struct device_node *np = dev->of_node;
	struct i2c_client *i2c = chg_data->chip->i2c;

	pr_info("%s\n", __func__);
	chg_data->chg_desc = &rt5081_default_chg_desc;

	chg_desc = devm_kzalloc(&i2c->dev,
		sizeof(struct rt5081_pmu_charger_desc), GFP_KERNEL);
	if (!chg_desc)
		return -ENOMEM;
	memcpy(chg_desc, &rt5081_default_chg_desc,
		sizeof(struct rt5081_pmu_charger_desc));

	if (of_property_read_string(np, "charger_name",
		&(chg_data->mchr_info.name)) < 0) {
		pr_err("%s: no charger name\n", __func__);
		chg_data->mchr_info.name = "rt5081_charger";
	}

#if 0 /* The unit of gm20 and gm30 is different, the dts now is for gm30 */
	if (of_property_read_u32(np, "ichg", &chg_desc->ichg) < 0)
		pr_err("%s: no ichg\n", __func__);

	if (of_property_read_u32(np, "aicr", &chg_desc->aicr) < 0)
		pr_err("%s: no aicr\n", __func__);

	if (of_property_read_u32(np, "mivr", &chg_desc->mivr) < 0)
		pr_err("%s: no mivr\n", __func__);

	if (of_property_read_u32(np, "cv", &chg_desc->cv) < 0)
		pr_err("%s: no cv\n", __func__);

	if (of_property_read_u32(np, "ieoc", &chg_desc->ieoc) < 0)
		pr_err("%s: no ieoc\n", __func__);

	if (of_property_read_u32(np, "safety_timer",
		&chg_desc->safety_timer) < 0)
		pr_err("%s: no safety timer\n", __func__);

	if (of_property_read_u32(np, "dc_wdt", &chg_desc->dc_wdt) < 0)
		pr_err("%s: no dc wdt\n", __func__);

	if (of_property_read_u32(np, "ircmp_resistor",
		&chg_desc->ircmp_resistor) < 0)
		pr_err("%s: no ircmp resistor\n", __func__);

	if (of_property_read_u32(np, "ircmp_vclamp",
		&chg_desc->ircmp_vclamp) < 0)
		pr_err("%s: no ircmp vclamp\n", __func__);
#endif

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

	pr_info("%s: (%s)\n", __func__, RT5081_PMU_CHARGER_DRV_VERSION);

	chg_data = devm_kzalloc(&pdev->dev, sizeof(*chg_data), GFP_KERNEL);
	if (!chg_data)
		return -ENOMEM;

	mutex_init(&chg_data->adc_access_lock);
	mutex_init(&chg_data->irq_access_lock);
	chg_data->chip = dev_get_drvdata(pdev->dev.parent);
	chg_data->dev = &pdev->dev;
	chg_data->chr_type = CHARGER_UNKNOWN;

	if (use_dt) {
		ret = rt_parse_dt(&pdev->dev, chg_data);
		if (ret < 0)
			pr_err("%s: parse dts failed\n", __func__);
	}
	platform_set_drvdata(pdev, chg_data);

	rt5081_pmu_charger_irq_register(pdev);

	/* Do initial setting */
	ret = rt5081_chg_init_setting(chg_data);
	if (ret < 0) {
		pr_err("%s: sw init failed\n", __func__);
		goto err_chg_init_setting;
	}

	/* SW workaround */
	ret = rt5081_chg_sw_workaround(chg_data);
	if (ret < 0) {
		pr_err("%s: software workaround failed\n", __func__);
		goto err_chg_sw_workaround;
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

	init_waitqueue_head(&chg_data->wait_queue);

	rt_charger_dump_register(&chg_data->mchr_info, NULL);

	chg_data->mchr_info.mchr_intf = rt5081_mchr_intf;
	mtk_charger_set_info(&chg_data->mchr_info);
	battery_charging_control = chr_control_interface;
	chargin_hw_init_done = KAL_TRUE;

	dev_info(&pdev->dev, "%s successfully\n", __func__);
	return ret;

err_chg_init_setting:
err_chg_sw_workaround:
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
MODULE_VERSION(RT5081_PMU_CHARGER_DRV_VERSION);

/*
 * Version Note
 * 1.0.6_MTK
 * (1) Use wait queue instead of mdelay for waiting interrupt event
 *
 * 1.0.5_MTK
 * (1) Modify USB charger type detecion flow
 * Follow notifier from TypeC if CONFIG_TCPC_CLASS is defined
 * (2) Add WDT timeout interrupt and kick watchdog in irq handler
 *
 * 1.0.4_MTK
 * (1) Add USB charger type detection
 *
 * 1.0.3_MTK
 * (1) Remove reset chip operation in probe function
 *
 * 1.0.2_MTK
 * (1) For normal boot, set IPREC to 850mA to prevent Isys drop
 * (2) Modify naming rules of functions
 *
 * 1.0.0_MTK
 * (1) Initial Release
 */
