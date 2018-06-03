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

/*
 *
 * Filename:
 * ---------
 *    mtk_charger.c
 *
 * Project:
 * --------
 *   Android_Software
 *
 * Description:
 * ------------
 *   This Module defines functions of Battery charging
 *
 * Author:
 * -------
 * Wy Chuang
 *
 */
#include <linux/init.h>		/* For init/exit macros */
#include <linux/module.h>	/* For MODULE_ marcros  */
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/power_supply.h>
#include <linux/wakelock.h>
#include <linux/time.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/scatterlist.h>
#include <linux/suspend.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/version.h>


#include "mtk_charger_intf.h"
#include <mt6336.h>
#include <upmu_common.h>

#define MTK_CHARGER_USE_POLLING

static struct mt6336_ctrl *lowq_ctrl;

static int mt6336_get_mivr(struct charger_device *chg_dev);

struct mt6336_charger {
	int i2c_log_level;
	struct power_supply_desc psd;
	struct power_supply *psy;

	const char *charger_dev_name;
	struct charger_properties charger_prop;
	struct charger_device *charger_dev;

};

/* mt6336 VCV_CON0[7:0], uV */
static const unsigned int VBAT_CV_VTH[] = {
	2600000, 2612500, 2625000, 2637500,
	2650000, 2662500, 2675000, 2687500,
	2700000, 2712500, 2725000, 2737500,
	2750000, 2762500, 2775000, 2787500,
	2800000, 2812500, 2825000, 2837500,
	2850000, 2862500, 2875000, 2887500,
	2900000, 2912500, 2925000, 2937500,
	2950000, 2962500, 2975000, 2987500,
	3000000, 3012500, 3025000, 3037500,
	3050000, 3062500, 3075000, 3087500,
	3100000, 3112500, 3125000, 3137500,
	3150000, 3162500, 3175000, 3187500,
	3200000, 3212500, 3225000, 3237500,
	3250000, 3262500, 3275000, 3287500,
	3300000, 3312500, 3325000, 3337500,
	3350000, 3362500, 3375000, 3387500,
	3400000, 3412500, 3425000, 3437500,
	3450000, 3462500, 3475000, 3487500,
	3500000, 3512500, 3525000, 3537500,
	3550000, 3562500, 3575000, 3587500,
	3600000, 3612500, 3625000, 3637500,
	3650000, 3662500, 3675000, 3687500,
	3700000, 3712500, 3725000, 3737500,
	3750000, 3762500, 3775000, 3787500,
	3800000, 3812500, 3825000, 3837500,
	3850000, 3862500, 3875000, 3887500,
	3900000, 3912500, 3925000, 3937500,
	3950000, 3962500, 3975000, 3987500,
	4000000, 4012500, 4025000, 4037500,
	4050000, 4062500, 4075000, 4087500,
	4100000, 4112500, 4125000, 4137500,
	4150000, 4162500, 4175000, 4187500,
	4200000, 4212500, 4225000, 4237500,
	4250000, 4262500, 4275000, 4287500,
	4300000, 4312500, 4325000, 4337500,
	4350000, 4362500, 4375000, 4387500,
	4400000, 4412500, 4425000, 4437500,
	4450000, 4462500, 4475000, 4487500,
	4500000, 4512500, 4525000, 4537500,
	4550000, 4562500, 4575000, 4587500,
	4600000, 4612500, 4625000, 4637500,
	4650000, 4662500, 4675000, 4687500,
	4700000, 4712500, 4725000, 4737500,
	4750000, 4762500, 4775000, 4787500,
	4800000,
};

/* mt6336 VRECHG_AUXADC, uV */
static const unsigned int VRECHG_AUXADC[2][169] = {
	{
	2600000, 2612500, 2625000, 2637500,
	2650000, 2662500, 2675000, 2687500,
	2700000, 2712500, 2725000, 2737500,
	2750000, 2762500, 2775000, 2787500,
	2800000, 2812500, 2825000, 2837500,
	2850000, 2862500, 2875000, 2887500,
	2900000, 2912500, 2925000, 2937500,
	2950000, 2962500, 2975000, 2987500,
	3000000, 3012500, 3025000, 3037500,
	3050000, 3062500, 3075000, 3087500,
	3100000, 3112500, 3125000, 3137500,
	3150000, 3162500, 3175000, 3187500,
	3200000, 3212500, 3225000, 3237500,
	3250000, 3262500, 3275000, 3287500,
	3300000, 3312500, 3325000, 3337500,
	3350000, 3362500, 3375000, 3387500,
	3400000, 3412500, 3425000, 3437500,
	3450000, 3462500, 3475000, 3487500,
	3500000, 3512500, 3525000, 3537500,
	3550000, 3562500, 3575000, 3587500,
	3600000, 3612500, 3625000, 3637500,
	3650000, 3662500, 3675000, 3687500,
	3700000, 3712500, 3725000, 3737500,
	3750000, 3762500, 3775000, 3787500,
	3800000, 3812500, 3825000, 3837500,
	3850000, 3862500, 3875000, 3887500,
	3900000, 3912500, 3925000, 3937500,
	3950000, 3962500, 3975000, 3987500,
	4000000, 4012500, 4025000, 4037500,
	4050000, 4062500, 4075000, 4087500,
	4100000, 4112500, 4125000, 4137500,
	4150000, 4162500, 4175000, 4187500,
	4200000, 4212500, 4225000, 4237500,
	4250000, 4262500, 4275000, 4287500,
	4300000, 4312500, 4325000, 4337500,
	4350000, 4362500, 4375000, 4387500,
	4400000, 4412500, 4425000, 4437500,
	4450000, 4462500, 4475000, 4487500,
	4500000, 4512500, 4525000, 4537500,
	4550000, 4562500, 4575000, 4587500,
	4600000, 4612500, 4625000, 4637500,
	4650000, 4662500, 4675000, 4687500,
	4700000,
	},
	{
	1972, 1982, 1991, 2001,
	2010, 2020, 2029, 2039,
	2048, 2057, 2067, 2076,
	2086, 2095, 2105, 2114,
	2124, 2133, 2143, 2152,
	2162, 2171, 2181, 2190,
	2200, 2209, 2219, 2228,
	2238, 2247, 2257, 2266,
	2276, 2285, 2295, 2304,
	2313, 2323, 2332, 2342,
	2351, 2361, 2370, 2380,
	2389, 2399, 2408, 2418,
	2427, 2437, 2446, 2456,
	2465, 2475, 2484, 2494,
	2503, 2513, 2522, 2532,
	2541, 2551, 2560, 2569,
	2579, 2588, 2598, 2607,
	2617, 2626, 2636, 2645,
	2655, 2664, 2674, 2683,
	2693, 2702, 2712, 2721,
	2731, 2740, 2750, 2759,
	2769, 2778, 2788, 2797,
	2807, 2816, 2825, 2835,
	2844, 2854, 2863, 2873,
	2882, 2892, 2901, 2911,
	2920, 2930, 2939, 2949,
	2958, 2968, 2977, 2987,
	2996, 3006, 3015, 3025,
	3034, 3044, 3053, 3063,
	3072, 3081, 3091, 3100,
	3110, 3119, 3129, 3138,
	3148, 3157, 3167, 3176,
	3186, 3195, 3205, 3214,
	3224, 3233, 3243, 3252,
	3262, 3271, 3281, 3290,
	3300, 3309, 3319, 3328,
	3337, 3347, 3356, 3366,
	3375, 3385, 3394, 3404,
	3413, 3423, 3432, 3442,
	3451, 3461, 3470, 3480,
	3489, 3499, 3508, 3518,
	3527, 3537, 3546, 3556,
	3565,
	},
};

/* mt6336 ICC_CON0[6:0], uA */
static const unsigned int CS_VTH[] = {
	500000, 550000, 600000, 650000,
	700000, 750000, 800000, 850000,
	900000, 950000, 1000000, 1050000,
	1100000, 1150000, 1200000, 1250000,
	1300000, 1350000, 1400000, 1450000,
	1500000, 1550000, 1600000, 1650000,
	1700000, 1750000, 1800000, 1850000,
	1900000, 1950000, 2000000, 2050000,
	2100000, 2150000, 2200000, 2250000,
	2300000, 2350000, 2400000, 2450000,
	2500000, 2550000, 2600000, 2650000,
	2700000, 2750000, 2800000, 2850000,
	2900000, 2950000, 3000000, 3050000,
	3100000, 3150000, 3200000, 3250000,
	3300000, 3350000, 3400000, 3450000,
	3500000, 3550000, 3600000, 3650000,
	3700000, 3750000, 3800000, 3850000,
	3900000, 3950000, 4000000, 4050000,
	4100000, 4150000, 4200000, 4250000,
	4300000, 4350000, 4400000, 4450000,
	4500000, 4550000, 4600000, 4650000,
	4700000, 4750000, 4800000, 4850000,
	4900000, 4950000, 5000000,
};

/* mt6336 ICL_CON0[5:0], uA */
static const unsigned int INPUT_CS_VTH[] = {
	50000, 100000, 150000, 500000,
	550000, 600000, 650000, 700000,
	750000, 800000, 850000, 900000,
	950000, 1000000, 1050000, 1100000,
	1150000, 1200000, 1250000, 1300000,
	1350000, 1400000, 1450000, 1500000,
	1550000, 1600000, 1650000, 1700000,
	1750000, 1800000, 1850000, 1900000,
	1950000, 2000000, 2050000, 2100000,
	2150000, 2200000, 2250000, 2300000,
	2350000, 2400000, 2450000, 2500000,
	2550000, 2600000, 2650000, 2700000,
	2750000, 2800000, 2850000, 2900000,
	2950000, 3000000, 3050000, 3100000,
	3150000, 3200000,
};

/* MT6336 GER_CON0[7:4], uA */
static const unsigned int ITERM_REG[] = {
	50000, 100000, 150000, 200000,
	250000, 300000, 350000, 400000,
	450000, 500000,
};

/* MT6336 GER_CON2[4:0], uV */
static const unsigned int MIVR_REG[] = {
	4300000, 4350000, 4400000, 4450000,
	4500000, 4550000, 4600000, 4650000,
	4700000, 4750000, 4800000, 4850000,
	4900000, 4900000, 4900000, 4900000,
	5500000, 6000000, 6500000, 7000000,
	7500000, 8000000, 8500000, 9000000,
	9500000, 10000000, 10500000, 11000000,
	11500000,
};

/* MT6336 OTG_CTRL2[2:0], uA */
static const unsigned int BOOST_CURRENT_LIMIT[] = {
	100000, 500000, 900000, 1200000,
	1500000, 2000000,
};

struct mt6336_charger *info;

static unsigned int charging_value_to_parameter(const unsigned int *parameter,
						const unsigned int array_size,
						const unsigned int val)
{
	if (val < array_size)
		return parameter[val];

	pr_err("Can't find the parameter\n");
	return parameter[0];

}

static unsigned int charging_parameter_to_value(const unsigned int *parameter,
						const unsigned int array_size,
						const unsigned int val)
{
	unsigned int i;

	for (i = 0; i < array_size; i++) {
		if (val == *(parameter + i))
			return i;
	}

	pr_err("NO register value match\n");
	return 0;
}

static unsigned int bmt_find_closest_level(const unsigned int *pList,
					   unsigned int number,
					   unsigned int level)
{
	int i;
	unsigned int max_value_in_last_element;

	if (pList[0] < pList[1])
		max_value_in_last_element = true;
	else
		max_value_in_last_element = false;

	if (max_value_in_last_element == true) {
		/* max value in the last element */
		for (i = (number - 1); i >= 0; i--) {
			if (pList[i] <= level) {
				/* pr_debug("zzf_%d<=%d i=%d\n", pList[i], level, i); */
				return pList[i];
			}
		}

		pr_debug("Can't find closest level\n");
		return pList[0];
	}

	/* max value in the first element */
	for (i = 0; i < number; i++) {
		if (pList[i] <= level)
			return pList[i];
	}

	pr_debug("Can't find closest level\n");
	return pList[number - 1];
}

static int mt6336_parse_dt(struct mt6336_charger *info, struct device *dev)
{
	struct device_node *np = dev->of_node;

	pr_err("%s: starts\n", __func__);

	if (!np) {
		pr_err("%s: no device node\n", __func__);
		return -EINVAL;
	}

	if (of_property_read_string(np, "charger_name",
		&info->charger_dev_name) < 0) {
		pr_err("%s: no charger name\n", __func__);
		info->charger_dev_name = "primary_chg";
	}

	if (of_property_read_string(np, "alias_name",
		&info->charger_prop.alias_name) < 0) {
		pr_err("%s: no alias name\n", __func__);
		info->charger_prop.alias_name = "mt6366";
	}

	pr_err("chr name:%s alias:%s\n",
		info->charger_dev_name, info->charger_prop.alias_name);

	return 0;
}

static enum power_supply_property mt6336_charger_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

char *mt6336_charger_supply_list[] = {
	"none",
};

static int mt6336_charger_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	/*struct mt6336_charger *info = dev_get_drvdata(psy->dev->parent);*/
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 1;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int mt6336_charger_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	/*struct mt6336_charger *info = dev_get_drvdata(psy->dev->parent);*/
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		break;
	default:
		break;
	}
	return ret;
}

int mt6336_plug_in_setting(struct charger_device *chg_dev)
{
	/* VCV/PAM status bias current setting */
	mt6336_config_interface(0x52A, 0x88, 0xFF, 0);
	/* Program the LG dead time control */
	mt6336_config_interface(0x553, 0x14, 0xFF, 0);
	/* Loop GM enable control */
	mt6336_config_interface(0x519, 0x3F, 0xFF, 0);
	mt6336_config_interface(0x51E, 0x02, 0xFF, 0);
	/* GM MSB */
	mt6336_config_interface(0x520, 0x04, 0xFF, 0);
	mt6336_config_interface(0x55A, 0x00, 0xFF, 0);
	mt6336_config_interface(0x455, 0x01, 0xFF, 0);
	mt6336_config_interface(0x3C9, 0x10, 0xFF, 0);
	mt6336_config_interface(0x3CF, 0x03, 0xFF, 0);
	/* ICC/ICL status bias current setting */
	mt6336_config_interface(0x529, 0x88, 0xFF, 0);
	/* Enable RG_EN_TERM */
	mt6336_set_flag_register_value(MT6336_RG_EN_TERM, 1);
	mt6336_set_flag_register_value(MT6336_RG_A_EN_ITERM, 1);

	return 0;
}

int mt6336_plug_out_setting(struct charger_device *chg_dev)
{
	/* Enable ctrl to lock power, keeping MT6336 in normal mode */
	mt6336_ctrl_enable(lowq_ctrl);
#ifndef MTK_CHARGER_USE_POLLING
	mt6336_disable_interrupt(MT6336_INT_CHR_BAT_RECHG, "mt6336 charger");
#endif
	mt6336_set_flag_register_value(MT6336_AUXADC_VBAT_IRQ_EN, 0);
	mt6336_set_flag_register_value(MT6336_RG_EN_RECHARGE, 0);
	mt6336_set_flag_register_value(MT6336_RG_EN_TERM, 0);
	mt6336_set_flag_register_value(MT6336_AUXADC_VBAT_VTH_MODE_SEL, 0);

	/* enter low power mode */
	mt6336_ctrl_disable(lowq_ctrl);

	return 0;
}

static int mt6336_enable_charging(struct charger_device *chg_dev, bool en)
{
	pr_err_ratelimited("%s: enable = %d\n", __func__, en);

	/* Enable ctrl to lock power, keeping MT6336 in normal mode */
	mt6336_ctrl_enable(lowq_ctrl);

	if (en) {
		mt6336_set_flag_register_value(MT6336_RG_EN_CHARGE, 1);
		mt6336_set_flag_register_value(MT6336_RG_A_LOOP_CLAMP_EN, 0);
		mt6336_set_flag_register_value(MT6336_RG_EN_TERM, 1);
		mt6336_set_flag_register_value(MT6336_RG_A_EN_ITERM, 1);
	} else {
		mt6336_set_flag_register_value(MT6336_RG_EN_CHARGE, 0);
#if 0
		mt6336_disable_interrupt(MT6336_INT_CHR_BAT_RECHG, "mt6336 charger");
		mt6336_set_flag_register_value(MT6336_AUXADC_VBAT_IRQ_EN, 0);
		mt6336_set_flag_register_value(MT6336_RG_EN_RECHARGE, 0);
		mt6336_set_flag_register_value(MT6336_RG_EN_TERM, 0);
		mt6336_set_flag_register_value(MT6336_AUXADC_VBAT_VTH_MODE_SEL, 0);
#endif
	}

	/* enter low power mode */
	mt6336_ctrl_disable(lowq_ctrl);

	return 0;
}

static int mt6336_is_charging_enabled(struct charger_device *chg_dev, bool *en)
{
	unsigned short val;

	pr_err("%s\n", __func__);
	val = mt6336_get_flag_register_value(MT6336_RG_EN_CHARGE);
	*en = (val == 0) ? false : true;

	return 0;
}

static int mt6336_enable_powerpath(struct charger_device *chg_dev, bool en)
{
	pr_debug("%s: enable = %d\n", __func__, en);

	if (en)
		mt6336_set_flag_register_value(MT6336_RG_EN_BUCK, 1);
	else
		mt6336_set_flag_register_value(MT6336_RG_EN_BUCK, 0);

	return 0;
}

static int mt6336_is_powerpath_enabled(struct charger_device *chg_dev, bool *en)
{
	unsigned short val;

	pr_debug("%s\n", __func__);
	val = mt6336_get_flag_register_value(MT6336_RG_EN_BUCK);
	*en = (val == 0) ? false : true;

	return 0;
}

static int pmic_enable_vbus_ovp(struct charger_device *chg_dev, bool en)
{
	pr_debug("%s %d\n", __func__, en);
	pmic_set_register_value(PMIC_RG_VCDT_HV_EN, en);

	return 0;
}

static int mt6336_get_ichg(struct charger_device *chg_dev, u32 *uA)
{
	unsigned int array_size;
	unsigned int val;

	/*Get current level */
	array_size = ARRAY_SIZE(CS_VTH);
	val = mt6336_get_flag_register_value(MT6336_RG_ICC);
	*uA = charging_value_to_parameter(CS_VTH, array_size, val);

	return 0;
}

static int mt6336_set_ichg(struct charger_device *chg_dev, u32 ichg)
{
	unsigned int set_ichg;
	unsigned int array_size;
	unsigned int register_value;

	array_size = ARRAY_SIZE(CS_VTH);
	set_ichg = bmt_find_closest_level(CS_VTH, array_size, ichg);
	register_value = charging_parameter_to_value(CS_VTH, array_size, set_ichg);
	if (register_value != 0x0)
		mt6336_set_flag_register_value(MT6336_RG_ICC, register_value - 1);
	mt6336_set_flag_register_value(MT6336_RG_ICC, register_value);

	pr_debug_ratelimited("%s: 0x%x %d %d\n", __func__, register_value, ichg, set_ichg);

	return 0;
}

static int mt6336_get_min_ichg(struct charger_device *chg_dev, u32 *uA)
{
	unsigned int array_size;

	array_size = ARRAY_SIZE(CS_VTH);
	*uA = charging_value_to_parameter(CS_VTH, array_size, 0);

	return 0;
}

static int mt6336_get_cv(struct charger_device *chg_dev, u32 *uV)
{
	unsigned int array_size;
	unsigned int val;

	array_size = ARRAY_SIZE(VBAT_CV_VTH);
	val = mt6336_get_flag_register_value(MT6336_RG_VCV);
	*uV = charging_value_to_parameter(VBAT_CV_VTH, array_size, val);

	return 0;
}

static int mt6336_set_cv(struct charger_device *chg_dev, u32 cv)
{
	unsigned short int array_size;
	unsigned int set_cv;
	unsigned short int register_value;
	unsigned int idx;

	array_size = ARRAY_SIZE(VBAT_CV_VTH);
	set_cv = bmt_find_closest_level(VBAT_CV_VTH, array_size, cv);
	register_value = charging_parameter_to_value(VBAT_CV_VTH, array_size, set_cv);
	mt6336_set_flag_register_value(MT6336_RG_VCV, register_value);

	/* pr_debug("%s: 0x%x %d %d\n", __func__, register_value, cv, set_cv); */

	/* Set VRECHG_AUXADC to 30mV less than CV */
	array_size = ARRAY_SIZE(VRECHG_AUXADC[0]);
	set_cv = bmt_find_closest_level(VRECHG_AUXADC[0], array_size, set_cv - 30000);
	idx = charging_parameter_to_value(VRECHG_AUXADC[0], array_size, set_cv);
	register_value = VRECHG_AUXADC[1][idx];

	mt6336_set_flag_register_value(MT6336_AUXADC_VBAT_DET_VOLT_11_0_H, 0xF);
	mt6336_set_flag_register_value(MT6336_AUXADC_VBAT_DET_VOLT_11_0_L,
					register_value & 0xFF);
	mt6336_set_flag_register_value(MT6336_AUXADC_VBAT_DET_VOLT_11_0_H,
					(register_value >> 8) & 0xF);

	pr_err("%s: rechg 0x%x %d %d\n", __func__, register_value, cv, set_cv);
	return 0;
}

static int mt6336_get_aicr(struct charger_device *chg_dev, u32 *uA)
{
	unsigned int array_size;
	unsigned int val;

	array_size = ARRAY_SIZE(INPUT_CS_VTH);
	val = mt6336_get_flag_register_value(MT6336_RG_ICL);
	*uA = charging_value_to_parameter(INPUT_CS_VTH, array_size, val);

	return 0;
}

static int mt6336_set_aicr(struct charger_device *chg_dev, u32 aicr)
{
	unsigned int set_aicr;
	unsigned int array_size;
	unsigned int register_value;

	array_size = ARRAY_SIZE(INPUT_CS_VTH);
	set_aicr = bmt_find_closest_level(INPUT_CS_VTH, array_size, aicr);
	register_value = charging_parameter_to_value(INPUT_CS_VTH, array_size, set_aicr);
	mt6336_set_flag_register_value(MT6336_RG_ICL, register_value);

	pr_debug_ratelimited("%s: 0x%x %d %d\n", __func__, register_value, aicr, set_aicr);

	return 0;
}

static int mt6336_get_min_aicr(struct charger_device *chg_dev, u32 *uA)
{
	unsigned int array_size;

	array_size = ARRAY_SIZE(INPUT_CS_VTH);
	*uA = charging_value_to_parameter(INPUT_CS_VTH, array_size, 0);

	return 0;
}

static int mt6336_get_iterm(struct charger_device *chg_dev, u32 *uA)
{
	unsigned int array_size;
	unsigned int val;

	array_size = ARRAY_SIZE(ITERM_REG);
	val = mt6336_get_flag_register_value(MT6336_RG_ITERM);
	*uA = charging_value_to_parameter(ITERM_REG, array_size, val);

	return 0;
}

static int mt6336_set_iterm(struct charger_device *chg_dev, u32 iterm)
{
	unsigned int set_iterm;
	unsigned int array_size;
	unsigned int register_value;

	array_size = ARRAY_SIZE(ITERM_REG);
	set_iterm = bmt_find_closest_level(ITERM_REG, array_size, iterm);
	register_value = charging_parameter_to_value(ITERM_REG, array_size, set_iterm);
	mt6336_set_flag_register_value(MT6336_RG_ITERM, register_value);

	pr_debug("%s: 0x%x %d %d\n", __func__, register_value, iterm, set_iterm);

	return 0;
}

static int mt6336_get_eoc(struct charger_device *chr_dev, bool *is_eoc)
{
	unsigned short vth_mode;

	*is_eoc = mt6336_get_flag_register_value(MT6336_DA_QI_EOC_STAT_MUX);
	vth_mode = mt6336_get_flag_register_value(MT6336_AUXADC_VBAT_VTH_MODE_SEL);
	pr_err("mt6336_get_eoc: %d %d\n", *is_eoc, vth_mode);
	return 0;
}

static int mt6336_dump_register(struct charger_device *chg_dev)
{
	unsigned short icc;
	u32 aicr = 0;
	unsigned short vcv;
	unsigned short chg_en;
	unsigned short state;
	unsigned short pam_state;
	unsigned short vth_mode;
	u32 vpam;
	u32 iterm;

	icc = mt6336_get_flag_register_value(MT6336_RG_ICC);
	mt6336_get_aicr(chg_dev, &aicr);
	vcv = mt6336_get_flag_register_value(MT6336_RG_VCV);
	mt6336_get_iterm(chg_dev, &iterm);
	chg_en = mt6336_get_flag_register_value(MT6336_RG_EN_CHARGE);
	state = mt6336_get_register_value(MT6336_PMIC_ANA_CORE_DA_RGS13);
	pam_state = mt6336_get_flag_register_value(MT6336_AD_QI_PAM_MODE);
	vpam = mt6336_get_mivr(chg_dev);
	vth_mode = mt6336_get_flag_register_value(MT6336_AUXADC_VBAT_VTH_MODE_SEL);

	pr_err("ICC:%dmA,ICL:%dmA,VCV:%dmV,Iterm:%dmA,VPAM:%dmV,EN=%d,state=%x,PAM:%d,mode:%d\n",
		icc * 50 + 500, aicr / 1000, (vcv * 125 + 26000) / 10, iterm / 1000,
		vpam / 1000, chg_en, state, pam_state, vth_mode);

	return 0;
}

int mt6336_set_pe20_efficiency_table(struct charger_device *chg_dev)
{
	struct charger_manager *pinfo;

	pinfo = charger_dev_get_drvdata(chg_dev);

	if (pinfo != NULL) {
		pinfo->pe2.profile[0].vchr = 8500000;
		pinfo->pe2.profile[1].vchr = 8500000;
		pinfo->pe2.profile[2].vchr = 8500000;
		pinfo->pe2.profile[3].vchr = 9000000;
		pinfo->pe2.profile[4].vchr = 9000000;
		pinfo->pe2.profile[5].vchr = 9500000;
		pinfo->pe2.profile[6].vchr = 9500000;
		pinfo->pe2.profile[7].vchr = 9500000;
		pinfo->pe2.profile[8].vchr = 10000000;
		pinfo->pe2.profile[9].vchr = 10000000;
		return 0;
	}
	return -1;
}

static int mt6336_send_ta20_current_pattern(struct charger_device *chg_dev, u32 uV)
{
	int pep_value;
	unsigned int fastcc_stat;

	fastcc_stat = mt6336_get_flag_register_value(MT6336_DA_QI_FASTCC_STAT_MUX);
	if (fastcc_stat != 1) {
		pr_err("Not in FASTCC state, cannot use PE+!\n");
		return -1;
	}

	/* Set charging current to 500mA */
	mt6336_set_aicr(chg_dev, 500000);
	mt6336_set_ichg(chg_dev, 500000);
	usleep_range(1000, 1200);

	/* Enable clock */
	mt6336_set_flag_register_value(MT6336_CLK_REG_6M_W1C_CK_PDN, 0x0);

	if (uV == 25000000)
		pep_value = 31;
	else
		pep_value = (uV - 5500000) / 500000;
	pr_debug("uv: %d, pep_value: 0x%x\n", uV, pep_value);

	mt6336_set_flag_register_value(MT6336_RG_PEP_DATA, pep_value);
	mt6336_set_flag_register_value(MT6336_RG_PE_SEL, 1);
	mt6336_set_flag_register_value(MT6336_RG_PE_TC_SEL, 2);
	mt6336_set_flag_register_value(MT6336_RG_PE_ENABLE, 1);

	msleep(1600);

	return 0;
}

static int mt6336_set_ta20_reset(struct charger_device *chg_dev)
{
	unsigned int val;

	mt6336_set_flag_register_value(MT6336_RG_EN_TERM, 0);

	val = mt6336_get_flag_register_value(MT6336_RG_ICL);
	mt6336_set_flag_register_value(MT6336_RG_ICL, 0x0);
	msleep(250);
	mt6336_set_flag_register_value(MT6336_RG_ICL, val);

	return 0;
}

static int mt6336_enable_cable_drop_comp(struct charger_device *chg_dev, bool en)
{
	int ret = 0;

	pr_info("%s: en = %d\n", __func__, en);
	if (!en)
		ret = mt6336_send_ta20_current_pattern(chg_dev, 25000000);

	return ret;
}

static int mt6336_set_mivr(struct charger_device *chg_dev, u32 uV)
{
	unsigned int set_mivr;
	unsigned int array_size;
	unsigned int register_value;

	array_size = ARRAY_SIZE(MIVR_REG);
	set_mivr = bmt_find_closest_level(MIVR_REG, array_size, uV);
	register_value = charging_parameter_to_value(MIVR_REG, array_size, set_mivr);
	mt6336_set_flag_register_value(MT6336_RG_VPAM, register_value);

	pr_err("%s: 0x%x %d %d\n", __func__, register_value, uV, set_mivr);

	return 0;
}

static int mt6336_get_mivr(struct charger_device *chg_dev)
{
	unsigned int array_size;
	unsigned int val;

	array_size = ARRAY_SIZE(MIVR_REG);
	val = mt6336_get_flag_register_value(MT6336_RG_VPAM);

	return charging_value_to_parameter(MIVR_REG, array_size, val);
}

static int mt6336_get_mivr_state(struct charger_device *chg_dev, bool *in_loop)
{
	*in_loop = mt6336_get_flag_register_value(MT6336_AD_QI_PAM_MODE);
	return 0;
}

static int mt6336_enable_safety_timer(struct charger_device *chg_dev, bool en)
{
	pr_debug("%s: enable = %d\n", __func__, en);
	if (en)
		mt6336_set_flag_register_value(MT6336_RG_EN_CHR_SAFETMR, 1);
	else
		mt6336_set_flag_register_value(MT6336_RG_EN_CHR_SAFETMR, 0);

	return 0;
}

static int mt6336_is_safety_timer_enabled(struct charger_device *chg_dev, bool *en)
{
	unsigned short val;

	pr_debug("%s\n", __func__);
	val = mt6336_get_flag_register_value(MT6336_RG_EN_CHR_SAFETMR);
	*en = (val == 0) ? false : true;

	return 0;
}

static int mt6336_enable_term(struct charger_device *chg_dev, bool en)
{
	pr_err("%s: enable = %d\n", __func__, en);

	if (en) {
		mt6336_disable_interrupt(MT6336_INT_CHR_ICHR_ITERM, "mt6336 charger");
#ifndef MTK_CHARGER_USE_POLLING
		mt6336_enable_interrupt(MT6336_INT_STATE_BUCK_EOC, "mt6336 charger");
#endif
		mt6336_set_flag_register_value(MT6336_RG_EN_TERM, 1);
	} else {
		mt6336_set_flag_register_value(MT6336_RG_EN_TERM, 0);
#ifndef MTK_CHARGER_USE_POLLING
		mt6336_disable_interrupt(MT6336_INT_STATE_BUCK_EOC, "mt6336 charger");
#endif
		mt6336_enable_interrupt(MT6336_INT_CHR_ICHR_ITERM, "mt6336 charger");
	}

	return 0;
}

static int mt6336_enable_otg(struct charger_device *chg_dev, bool en)
{
	int ret = 0;
	unsigned short flash_sta, vbus_plugin;

	pr_debug("%s: en = %d\n", __func__, en);

	flash_sta = mt6336_get_flag_register_value(MT6336_DA_QI_FLASH_MODE_MUX);
	if (flash_sta) {
		pr_err("At Flash mode, cannot turn on OTG\n");
		return -EBUSY;
	}

	vbus_plugin = mt6336_get_flag_register_value(MT6336_DA_QI_VBUS_PLUGIN_MUX);

	if (vbus_plugin) {
		pr_err("VBUS present, cannot turn on OTG\n");
		return -EBUSY;
	}

	/* Enable ctrl to lock power, keeping MT6336 in normal mode */
	mt6336_ctrl_enable(lowq_ctrl);

	if (en) {
		mt6336_config_interface(0x519, 0x0D, 0xFF, 0);
		mt6336_config_interface(0x520, 0x00, 0xFF, 0);
		mt6336_config_interface(0x55A, 0x01, 0xFF, 0);
		mt6336_config_interface(0x455, 0x00, 0xFF, 0);
		mt6336_config_interface(0x3C9, 0x00, 0xFF, 0);
		mt6336_config_interface(0x3CF, 0x00, 0xFF, 0);
		mt6336_config_interface(0x553, 0x14, 0xFF, 0);
		mt6336_config_interface(0x55F, 0xE6, 0xFF, 0);
		mt6336_config_interface(0x53D, 0x47, 0xFF, 0);
		mt6336_config_interface(0x529, 0x8E, 0xFF, 0);
		mt6336_config_interface(0x560, 0x0C, 0xFF, 0);
		mt6336_config_interface(0x40F, 0x04, 0xFF, 0);

		mt6336_set_flag_register_value(MT6336_RG_EN_OTG, 1);
	} else {
		mt6336_config_interface(0x52A, 0x88, 0xFF, 0);
		mt6336_config_interface(0x553, 0x14, 0xFF, 0);
		mt6336_config_interface(0x519, 0x3F, 0xFF, 0);
		mt6336_config_interface(0x51E, 0x02, 0xFF, 0);
		mt6336_config_interface(0x520, 0x04, 0xFF, 0);
		mt6336_config_interface(0x55A, 0x00, 0xFF, 0);
		mt6336_config_interface(0x455, 0x01, 0xFF, 0);
		mt6336_config_interface(0x3C9, 0x10, 0xFF, 0);
		mt6336_config_interface(0x3CF, 0x03, 0xFF, 0);
		/* mt6336_config_interface(0x5AF, 0x02, 0xFF, 0); */
		/* mt6336_config_interface(0x64E, 0x02, 0xFF, 0); */
		mt6336_config_interface(0x402, 0x03, 0xFF, 0);
		mt6336_config_interface(0x529, 0x88, 0xFF, 0);

		mt6336_set_flag_register_value(MT6336_RG_EN_OTG, 0);
	}

	/* enter low power mode */
	mt6336_ctrl_disable(lowq_ctrl);

	return ret;
}

static int mt6336_set_boost_current_limit(struct charger_device *chg_dev, u32 uA)
{
	int ret = 0;
	u32 array_size = 0;
	u32 boost_ilimit = 0, boost_reg = 0;

	array_size = ARRAY_SIZE(BOOST_CURRENT_LIMIT);
	boost_ilimit = bmt_find_closest_level(BOOST_CURRENT_LIMIT, array_size, uA);
	boost_reg = charging_parameter_to_value(BOOST_CURRENT_LIMIT, array_size, boost_ilimit);
	mt6336_set_flag_register_value(MT6336_RG_OTG_IOLP, boost_reg);

	return ret;
}

void mt6336_buck_power_on_callback(void)
{
	pr_err("mt6336_buck_power_on_callback\n");
	mt6336_set_flag_register_value(MT6336_RG_EN_TERM, 1);
	mt6336_set_flag_register_value(MT6336_RG_A_EN_ITERM, 1);
}

void mt6336_buck_protect_callback(void)
{
	pr_err("mt6336_buck_protect_callback\n");
#ifndef MTK_CHARGER_USE_POLLING
	mt6336_disable_interrupt(MT6336_INT_CHR_BAT_RECHG, "mt6336 charger");
#endif
	mt6336_set_flag_register_value(MT6336_AUXADC_VBAT_IRQ_EN, 0);
	mt6336_set_flag_register_value(MT6336_RG_EN_RECHARGE, 0);
	mt6336_set_flag_register_value(MT6336_AUXADC_VBAT_VTH_MODE_SEL, 0);
	usleep_range(2000, 3000);
	mt6336_set_flag_register_value(MT6336_RG_A_EN_ITERM, 1);
}

void mt6336_chr_suspend_callback(void)
{
	pr_err("mt6336_chr_suspend_callback\n");
#ifndef MTK_CHARGER_USE_POLLING
	mt6336_disable_interrupt(MT6336_INT_CHR_BAT_RECHG, "mt6336 charger");
#endif
	mt6336_set_flag_register_value(MT6336_AUXADC_VBAT_IRQ_EN, 0);
	mt6336_set_flag_register_value(MT6336_RG_EN_RECHARGE, 0);
	mt6336_set_flag_register_value(MT6336_RG_EN_TERM, 0);
	mt6336_set_flag_register_value(MT6336_AUXADC_VBAT_VTH_MODE_SEL, 0);
}

void mt6336_chr_iterm_callback(void)
{
	pr_err("mt6336_chr_iterm_callback\n");

	if (info != NULL)
		charger_dev_notify(info->charger_dev, CHARGER_DEV_NOTIFY_EOC);
	else
		pr_err("do not call chain\n");
}

void mt6336_vbat_ovp_callback(void)
{
	pr_err("mt6336_vbat_ovp_callback\n");
	if (info != NULL)
		charger_dev_notify(info->charger_dev, CHARGER_DEV_NOTIFY_BAT_OVP);
	else
		pr_err("do not call chain\n");
}

void mt6336_vbus_ovp_callback(void)
{
	pr_err("mt6336_vbus_ovp_callback\n");
	if (info != NULL)
		charger_dev_notify(info->charger_dev, CHARGER_DEV_NOTIFY_VBUS_OVP);
}

void mt6336_eoc_callback(void)
{
	pr_err("mt6336_eoc_callback\n");

	mt6336_set_flag_register_value(MT6336_RG_A_EN_ITERM, 0);
	mt6336_set_flag_register_value(MT6336_AUXADC_VBAT_VTH_MODE_SEL, 1);
	mt6336_set_flag_register_value(MT6336_AUXADC_VBAT_EN_MODE_SEL, 0);
	mt6336_set_flag_register_value(MT6336_AUXADC_VBAT_EN_MODE_SEL, 1);
	mt6336_set_flag_register_value(MT6336_RG_EN_RECHARGE, 1);
	mt6336_set_flag_register_value(MT6336_AUXADC_VBAT_IRQ_EN, 1);
#ifndef MTK_CHARGER_USE_POLLING
	mt6336_enable_interrupt(MT6336_INT_CHR_BAT_RECHG, "mt6336 charger");
#endif

	if (info != NULL)
		charger_dev_notify(info->charger_dev, CHARGER_DEV_NOTIFY_EOC);
	else
		pr_err("do not call chain\n");
}

void mt6336_rechg_callback(void)
{
	pr_err("mt6336_rechg_callback\n");
#ifndef MTK_CHARGER_USE_POLLING
	mt6336_disable_interrupt(MT6336_INT_CHR_BAT_RECHG, "mt6336 charger");
#endif
	mt6336_set_flag_register_value(MT6336_AUXADC_VBAT_IRQ_EN, 0);
	mt6336_set_flag_register_value(MT6336_RG_EN_RECHARGE, 0);
	mt6336_set_flag_register_value(MT6336_AUXADC_VBAT_VTH_MODE_SEL, 0);
	usleep_range(2000, 3000);
	mt6336_set_flag_register_value(MT6336_RG_A_EN_ITERM, 1);

	if (info != NULL)
		charger_dev_notify(info->charger_dev, CHARGER_DEV_NOTIFY_RECHG);
	else
		pr_err("do not call chain\n");
}

void mt6336_safety_timeout_callback(void)
{
	pr_err("mt6336_safety_timeout_callback\n");
	mt6336_set_flag_register_value(MT6336_CLK_REG_6M_W1C_CK_PDN, 0x0);
	mt6336_set_flag_register_value(MT6336_RG_CHR_SAFETMR_CLEAR, 1);

	if (info != NULL)
		charger_dev_notify(info->charger_dev, CHARGER_DEV_NOTIFY_SAFETY_TIMEOUT);
	else
		pr_err("do not call chain\n");
}

int mt6336_event(struct charger_device *chg_dev, u32 event, u32 args)
{
	switch (event) {
	case EVENT_EOC:
			mt6336_eoc_callback();
		break;
	case EVENT_RECHARGE:
			mt6336_rechg_callback();
		break;
	default:
		break;
	}
	return 0;
}

static struct charger_ops mt6366_charger_dev_ops = {
	.suspend = NULL,
	.resume = NULL,
	.plug_in = mt6336_plug_in_setting,
	.plug_out = mt6336_plug_out_setting,
	.enable = mt6336_enable_charging,
	.is_enabled = mt6336_is_charging_enabled,
	.enable_powerpath = mt6336_enable_powerpath,
	.is_powerpath_enabled = mt6336_is_powerpath_enabled,
	.enable_vbus_ovp = pmic_enable_vbus_ovp,
	.get_charging_current = mt6336_get_ichg,
	.set_charging_current = mt6336_set_ichg,
	.get_min_charging_current = mt6336_get_min_ichg,
	.get_constant_voltage = mt6336_get_cv,
	.set_constant_voltage = mt6336_set_cv,
	.get_input_current = mt6336_get_aicr,
	.set_input_current = mt6336_set_aicr,
	.get_min_input_current = mt6336_get_min_aicr,
	.get_eoc_current = mt6336_get_iterm,
	.set_eoc_current = mt6336_set_iterm,
	.dump_registers = mt6336_dump_register,
	.is_charging_done = mt6336_get_eoc,
	.set_pe20_efficiency_table = mt6336_set_pe20_efficiency_table,
	.send_ta20_current_pattern = mt6336_send_ta20_current_pattern,
	.set_ta20_reset = mt6336_set_ta20_reset,
	.enable_cable_drop_comp = mt6336_enable_cable_drop_comp,
	.set_mivr = mt6336_set_mivr,
	.get_mivr_state = mt6336_get_mivr_state,
	.enable_safety_timer = mt6336_enable_safety_timer,
	.is_safety_timer_enabled = mt6336_is_safety_timer_enabled,
	.enable_termination = mt6336_enable_term,
	.event = mt6336_event,
	.enable_otg = mt6336_enable_otg,
	.set_boost_current_limit = mt6336_set_boost_current_limit,
};

static int mt6336_charger_probe(struct platform_device *pdev)
{
	int ret = 0;

	pr_err("%s: starts\n", __func__);

	info = devm_kzalloc(&pdev->dev, sizeof(struct mt6336_charger), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	mt6336_parse_dt(info, &pdev->dev);
	platform_set_drvdata(pdev, info);

#if 0
	info->psy.name = info->charger_dev_name;
	info->psy.type = POWER_SUPPLY_TYPE_UNKNOWN;
	info->psy.supplied_to = mt6336_charger_supply_list;
	info->psy.properties = mt6336_charger_props;
	info->psy.num_properties = ARRAY_SIZE(mt6336_charger_props);
	info->psy.get_property = mt6336_charger_get_property;
	info->psy.set_property = mt6336_charger_set_property;
#else
	info->psd.name = info->charger_dev_name;
	info->psd.type = POWER_SUPPLY_TYPE_UNKNOWN;
	info->psd.properties = mt6336_charger_props;
	info->psd.num_properties = ARRAY_SIZE(mt6336_charger_props);
	info->psd.get_property = mt6336_charger_get_property;
	info->psd.set_property = mt6336_charger_set_property;

	/*info->psy->supplied_to = mt6336_charger_supply_list;*/
#endif

	/*ret = power_supply_register(&pdev->dev, &info->psy);*/
	info->psy = power_supply_register(&pdev->dev, &info->psd, NULL);
	if (IS_ERR(info->psy)) {
		pr_err("mt6336 power supply regiseter fail\n");
		ret = -EINVAL;
		goto err_register_psy;
	}

	/* Register charger device */
	info->charger_dev = charger_device_register(info->charger_dev_name,
		&pdev->dev, info, &mt6366_charger_dev_ops, &info->charger_prop);
	if (IS_ERR_OR_NULL(info->charger_dev)) {
		ret = PTR_ERR(info->charger_dev);
		goto err_register_charger_dev;
	}

	info->charger_dev->is_polling_mode = true;

	lowq_ctrl = mt6336_ctrl_get("mt6336_charger");

	/* Enable ctrl to lock power, keeping MT6336 in normal mode */
	mt6336_ctrl_enable(lowq_ctrl);

	mt6336_set_flag_register_value(MT6336_RG_EN_CHARGE, 1);

	mt6336_set_mivr(info->charger_dev, 4500000);

	mt6336_register_interrupt_callback(MT6336_INT_CHR_BAT_OVP, mt6336_vbat_ovp_callback);
	mt6336_register_interrupt_callback(MT6336_INT_CHR_VBUS_OVP, mt6336_vbus_ovp_callback);
#ifndef MTK_CHARGER_USE_POLLING
	mt6336_register_interrupt_callback(MT6336_INT_STATE_BUCK_EOC, mt6336_eoc_callback);
	mt6336_register_interrupt_callback(MT6336_INT_CHR_BAT_RECHG, mt6336_rechg_callback);
#endif
	mt6336_register_interrupt_callback(MT6336_INT_SAFETY_TIMEOUT, mt6336_safety_timeout_callback);
	mt6336_register_interrupt_callback(MT6336_INT_DD_SWCHR_BUCK_MODE, mt6336_buck_power_on_callback);
	mt6336_register_interrupt_callback(MT6336_INT_DD_SWCHR_BUCK_PROTECT_STATE, mt6336_buck_protect_callback);
	mt6336_register_interrupt_callback(MT6336_INT_DD_SWCHR_CHR_SUSPEND_STATE, mt6336_chr_suspend_callback);
	mt6336_register_interrupt_callback(MT6336_INT_CHR_ICHR_ITERM, mt6336_chr_iterm_callback);

	mt6336_unmask_interrupt(MT6336_INT_CHR_BAT_OVP, "mt6336 charger");
	mt6336_unmask_interrupt(MT6336_INT_CHR_VBUS_OVP, "mt6336 charger");
#ifndef MTK_CHARGER_USE_POLLING
	mt6336_unmask_interrupt(MT6336_INT_STATE_BUCK_EOC, "mt6336 charger");
	mt6336_unmask_interrupt(MT6336_INT_CHR_BAT_RECHG, "mt6336 charger");
#endif
	mt6336_unmask_interrupt(MT6336_INT_SAFETY_TIMEOUT, "mt6336 charger");
	mt6336_unmask_interrupt(MT6336_INT_DD_SWCHR_BUCK_MODE, "mt6336 charger");
	mt6336_unmask_interrupt(MT6336_INT_DD_SWCHR_BUCK_PROTECT_STATE, "mt6336 charger");
	mt6336_unmask_interrupt(MT6336_INT_DD_SWCHR_CHR_SUSPEND_STATE, "mt6336 charger");
	mt6336_unmask_interrupt(MT6336_INT_CHR_ICHR_ITERM, "mt6336 charger");

	mt6336_enable_interrupt(MT6336_INT_CHR_BAT_OVP, "mt6336 charger");
	mt6336_enable_interrupt(MT6336_INT_CHR_VBUS_OVP, "mt6336 charger");
#ifndef MTK_CHARGER_USE_POLLING
	mt6336_enable_interrupt(MT6336_INT_STATE_BUCK_EOC, "mt6336 charger");
	/* mt6336_enable_interrupt(MT6336_INT_CHR_BAT_RECHG, "mt6336 charger"); */
#endif
	mt6336_enable_interrupt(MT6336_INT_SAFETY_TIMEOUT, "mt6336 charger");
	mt6336_enable_interrupt(MT6336_INT_DD_SWCHR_BUCK_MODE, "mt6336 charger");
	mt6336_enable_interrupt(MT6336_INT_DD_SWCHR_BUCK_PROTECT_STATE, "mt6336 charger");
	mt6336_enable_interrupt(MT6336_INT_DD_SWCHR_CHR_SUSPEND_STATE, "mt6336 charger");
	mt6336_disable_interrupt(MT6336_INT_CHR_ICHR_ITERM, "mt6336 charger");

	/* enter low power mode */
	mt6336_ctrl_disable(lowq_ctrl);

	return 0;

err_register_charger_dev:
	power_supply_unregister(info->psy);
err_register_psy:
	devm_kfree(&pdev->dev, info);
	return ret;

}

static int mt6336_charger_remove(struct platform_device *pdev)
{
	struct mt6336_charger *mt = platform_get_drvdata(pdev);

	if (mt) {
		power_supply_unregister(mt->psy);
		devm_kfree(&pdev->dev, mt);
	}
	return 0;
}

static void mt6336_charger_shutdown(struct platform_device *dev)
{
}


static const struct of_device_id mt6336_charger_of_match[] = {
	{.compatible = "mediatek,mt6336_charger",},
	{},
};

MODULE_DEVICE_TABLE(of, mt6336_charger_of_match);

static struct platform_driver mt6336_charger_driver = {
	.probe = mt6336_charger_probe,
	.remove = mt6336_charger_remove,
	.shutdown = mt6336_charger_shutdown,
	.driver = {
		   .name = "mt6336_charger",
		   .of_match_table = mt6336_charger_of_match,
		   },
};

static int __init mt6336_charger_init(void)
{
	return platform_driver_register(&mt6336_charger_driver);
}
module_init(mt6336_charger_init);

static void __exit mt6336_charger_exit(void)
{
	platform_driver_unregister(&mt6336_charger_driver);
}
module_exit(mt6336_charger_exit);

MODULE_AUTHOR("wy.chuang <wy.chuang@mediatek.com>");
MODULE_DESCRIPTION("MTK Switch Charger Device Driver");
MODULE_LICENSE("GPL");
