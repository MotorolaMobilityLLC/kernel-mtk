/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _PMIC_SW_H_
#define _PMIC_SW_H_


#define PMIC_DEBUG

#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
/*
 * The CHIP INFO
 */
#define PMIC6351_E1_CID_CODE    0x5110
#define PMIC6351_E2_CID_CODE    0x5120
#define PMIC6351_E3_CID_CODE    0x5130


#define pmic_emerg(fmt, args...)		pr_emerg("[SPM-PMIC] " fmt, ##args)
#define pmic_alert(fmt, args...)		pr_alert("[SPM-PMIC] " fmt, ##args)
#define pmic_crit(fmt, args...)		pr_crit("[SPM-PMIC] " fmt, ##args)
#define pmic_err(fmt, args...)		pr_err("[SPM-PMIC] " fmt, ##args)
#define pmic_warn(fmt, args...)		pr_warn("[SPM-PMIC] " fmt, ##args)
#define pmic_notice(fmt, args...)	pr_notice("[SPM-PMIC] " fmt, ##args)
#define pmic_info(fmt, args...)		pr_info("[SPM-PMIC] " fmt, ##args)
#define pmic_debug(fmt, args...)		pr_debug("[SPM-PMIC] " fmt, ##args)	/* pr_debug show nothing */

/* just use in suspend flow for important log due to console suspend */
#if defined PMIC_DEBUG_PR_DBG
#define pmic_spm_crit2(fmt, args...)		\
do {					\
	aee_sram_printk(fmt, ##args);	\
	pmic_crit(fmt, ##args);		\
} while (0)
#else
#define pmic_spm_crit2(fmt, args...)		\
do {					\
	aee_sram_printk(fmt, ##args);	\
} while (0)
#endif

/*#define PMIC_DEBUG_PR_DBG*/

#define PMICTAG                "[PMIC] "
#ifdef PMIC_DEBUG
#define PMICDEB(fmt, arg...) pr_debug(PMICTAG "cpuid=%d, " fmt, raw_smp_processor_id(), ##arg)
#define PMICFUC(fmt, arg...) pr_debug(PMICTAG "cpuid=%d, %s\n", raw_smp_processor_id(), __func__)
#endif  /*-- defined PMIC_DEBUG --*/
#if defined PMIC_DEBUG_PR_DBG
#define PMICLOG(fmt, arg...)   pr_err(PMICTAG fmt, ##arg)
#else
#define PMICLOG(fmt, arg...)
#endif  /*-- defined PMIC_DEBUG_PR_DBG --*/
#define PMICERR(fmt, arg...)   pr_debug(PMICTAG "ERROR,line=%d " fmt, __LINE__, ##arg)
#define PMICREG(fmt, arg...)   pr_debug(PMICTAG fmt, ##arg)

#define PMIC_EN REGULATOR_CHANGE_STATUS
#define PMIC_VOL REGULATOR_CHANGE_VOLTAGE
#define PMIC_EN_VOL 9

#define PMIC_INT_WIDTH 16

#define GETSIZE(array) (sizeof(array)/sizeof(array[0]))

/* PMIC EXTERN VARIABLE */
/*----- LOW_BATTERY_PROTECT -----*/
extern int g_lowbat_int_bottom;
extern int g_low_battery_level;
/*----- BATTERY_OC_PROTECT -----*/
extern int g_battery_oc_level;

/* PMIC EXTERN FUNCTIONS */
/*----- LOW_BATTERY_PROTECT -----*/
extern void lbat_min_en_setting(int en_val);
extern void lbat_max_en_setting(int en_val);
extern void exec_low_battery_callback(LOW_BATTERY_LEVEL low_battery_level);
/*----- BATTERY_OC_PROTECT -----*/
extern void exec_battery_oc_callback(BATTERY_OC_LEVEL battery_oc_level);
extern void bat_oc_h_en_setting(int en_val);
extern void bat_oc_l_en_setting(int en_val);
/*----- CHRDET_PROTECT -----*/
#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
extern unsigned int upmu_get_rgs_chrdet(void);
#endif
/*----- PMIC thread -----*/
extern void pmic_enable_charger_detection_int(int x);
/*----- LDO OC  -----*/
extern void msdc_sd_power_off(void);

/* extern functions */
extern void mt_power_off(void);
extern const PMU_FLAG_TABLE_ENTRY pmu_flags_table[];
extern unsigned int bat_get_ui_percentage(void);
extern signed int fgauge_read_IM_current(void *data);
extern void pmic_auxadc_lock(void);
extern void pmic_auxadc_unlock(void);
extern unsigned int bat_get_ui_percentage(void);
extern signed int fgauge_read_v_by_d(int d_val);
extern signed int fgauge_read_r_bat_by_v(signed int voltage);
/*extern PMU_ChargerStruct BMT_status;*//*have defined in battery_common.h */
extern void kpd_pwrkey_pmic_handler(unsigned long pressed);
extern void kpd_pmic_rstkey_handler(unsigned long pressed);
extern int is_mt6311_sw_ready(void);
extern int is_mt6311_exist(void);
extern int get_mt6311_i2c_ch_num(void);
extern bool crystal_exist_status(void);
#if !defined CONFIG_MTK_LEGACY
extern void pmu_drv_tool_customization_init(void);
#endif
extern int batt_init_cust_data(void);

extern unsigned int mt_gpio_to_irq(unsigned int gpio);
extern int mt_gpio_set_debounce(unsigned gpio, unsigned debounce);
/*---------------------------------------------------*/

/* controllable voltage , not fixed step */
#define PMIC_LDO_GEN1(_name, en, vol, array, use, mode)	\
	{	\
		.desc = {	\
			.name = #_name,	\
			.n_voltages = (sizeof(array)/sizeof(array[0])),	\
			.ops = &mtk_regulator_ops,	\
			.type = REGULATOR_VOLTAGE,	\
		},	\
		.init_data = {	\
			.constraints = {	\
				.valid_ops_mask = (mode),	\
			},	\
		},	\
		.en_att = __ATTR(LDO_##_name##_STATUS, 0664, show_LDO_STATUS, store_LDO_STATUS),	\
		.voltage_att = __ATTR(LDO_##_name##_VOLTAGE, 0664, show_LDO_VOLTAGE, store_LDO_VOLTAGE),	\
		.pvoltages = (void *)(array),	\
		.en_reg = (PMU_FLAGS_LIST_ENUM)(en),	\
		.vol_reg = (PMU_FLAGS_LIST_ENUM)(vol),	\
		.isUsedable = (use),	\
	}
/* controllable voltage , fixed step */
#define PMIC_LDO_GEN2(_name, en, vol, min, max, step, use, mode)	\
	{	\
		.desc = {	\
			.name = #_name,	\
			.n_voltages = ((max) - (min)) / (step) + 1,	\
			.ops = &mtk_regulator_ops,	\
			.type = REGULATOR_VOLTAGE,	\
			.min_uV = (min),	\
			.uV_step = (step),	\
		},	\
		.init_data = {	\
			.constraints = {	\
				.valid_ops_mask = (mode),	\
			},	\
		},	\
		.en_att = __ATTR(LDO_##_name##_STATUS, 0664, show_LDO_STATUS, store_LDO_STATUS),	\
		.voltage_att = __ATTR(LDO_##_name##_VOLTAGE, 0664, show_LDO_VOLTAGE, store_LDO_VOLTAGE),	\
		.en_reg = (en),	\
		.vol_reg = (vol),	\
		.isUsedable = (use),	\
	}

#define PMIC_BUCK_GEN(_name, en, vol, min, max, step)	\
	{	\
		.desc = {	\
			.name = #_name,	\
			.n_voltages = ((max) - (min)) / (step) + 1,	\
			.min_uV = (min),	\
			.uV_step = (step),	\
		},	\
		.en_att = __ATTR(BUCK_##_name##_STATUS, 0664, show_BUCK_STATUS, store_BUCK_STATUS),	\
		.voltage_att = __ATTR(BUCK_##_name##_VOLTAGE, 0664, show_BUCK_VOLTAGE, store_BUCK_VOLTAGE),	\
		.qi_en_reg = (en),	\
		.qi_vol_reg = (vol),	\
		.isUsedable = 0,	\
	}

/* fixed voltage */
#define PMIC_LDO_GEN3(_name, en, fixvoltage, use, mode)	\
	{	\
		.desc = {	\
			.name = #_name,	\
			.n_voltages = 1,	\
			.ops = &mtk_regulator_ops,	\
			.type = REGULATOR_VOLTAGE,	\
		},	\
		.init_data = {	\
			.constraints = {	\
				.valid_ops_mask = (mode),	\
			},	\
		},	\
		.en_att = __ATTR(LDO_##_name##_STATUS, 0664, show_LDO_STATUS, store_LDO_STATUS),	\
		.voltage_att = __ATTR(LDO_##_name##_VOLTAGE, 0664, show_LDO_VOLTAGE, store_LDO_VOLTAGE),	\
		.en_reg = (en),	\
		.fixedVoltage_uv = (fixvoltage),	\
		.isUsedable = (use),	\
	}

struct mtk_regulator_vosel {
	unsigned int def_sel; /*-- default vosel --*/
	unsigned int cur_sel; /*-- current vosel --*/
	bool restore;
};

struct mtk_regulator {
	struct regulator_desc desc;
	struct regulator_init_data init_data;
	struct regulator_config config;
	struct device_attribute en_att;
	struct device_attribute voltage_att;
	struct regulator_dev *rdev;
	PMU_FLAGS_LIST_ENUM en_reg;
	PMU_FLAGS_LIST_ENUM vol_reg;
	PMU_FLAGS_LIST_ENUM qi_en_reg;
	PMU_FLAGS_LIST_ENUM qi_vol_reg;
	const void *pvoltages;
	bool isUsedable;
	struct regulator *reg;
	/*--- Add to record selector ---*/
	struct mtk_regulator_vosel vosel;
	int vsleep_en_saved;
};



#define PMIC_INTERRUPT_WIDTH 16

#define PMIC_S_INT_GEN(_name)	\
	{	\
		.name =  #_name,	\
	}

#define PMIC_M_INTS_GEN(adr, enA, setA, clearA, interrupt)	\
	{	\
		.address =  adr,	\
		.en =  enA,	\
		.set =  setA,	\
		.clear =  clearA,	\
		.interrupts = interrupt,	\
	}

struct pmic_interrupt_bit {
	const char *name;
	void (*callback)(void);
	unsigned int times;
};

struct pmic_interrupts {
	unsigned int address;
	unsigned int en;
	unsigned int set;
	unsigned int clear;
	struct pmic_interrupt_bit *interrupts;
};


/* controllable voltage , not fixed step */
/*
#define PMIC_LDO_GEN1(_name, en, vol, array, use)	\
	{	\
		.desc = {	\
			.name = #_name,	\
			.n_voltages = (sizeof(array)/sizeof(array[0])),	\
			.ops = &mtk_regulator_ops,	\
			.type = REGULATOR_VOLTAGE,	\
			.owner = THIS_MODULE,	\
		},	\
		.en_att = __ATTR(LDO_##_name##_STATUS, 0664, show_LDO_STATUS, store_LDO_STATUS),	\
		.voltage_att = __ATTR(LDO_##_name##_VOLTAGE, 0664, show_LDO_VOLTAGE, store_LDO_VOLTAGE),	\
		.pvoltages = array,	\
		.en_reg = (en),	\
		.vol_reg = (vol),	\
		.isUsedable = (use),	\
	}
*/
/* fixed voltage */
/*
#define PMIC_LDO_GEN2(_name, en, fixvoltage, use)	\
	{	\
		.desc = {	\
			.name = #_name,	\
			.n_voltages = 1,	\
			.ops = &mtk_regulator_ops,	\
			.type = REGULATOR_VOLTAGE,	\
			.owner = THIS_MODULE,	\
		},	\
		.en_att = __ATTR(LDO_##_name##_STATUS, 0664, show_LDO_STATUS, store_LDO_STATUS),	\
		.voltage_att = __ATTR(LDO_##_name##_VOLTAGE, 0664, show_LDO_VOLTAGE, store_LDO_VOLTAGE),	\
		.en_reg = (en),	\
		.fixedVoltage_uv = (fixvoltage),	\
		.isUsedable = (use),	\
	}
*/
/* controllable voltage , fixed step */
/*
#define PMIC_LDO_GEN3(_name, en, vol, min, max, step, use)	\
	{	\
		.desc = {	\
			.name = #_name,	\
			.n_voltages = ((max) - (min)) / (step) + 1,	\
			.ops = &mtk_regulator_ops,	\
			.type = REGULATOR_VOLTAGE,	\
			.owner = THIS_MODULE,	\
			.min_uV = (min),	\
			.uV_step = (step),	\
		},	\
		.en_att = __ATTR(LDO_##_name##_STATUS, 0664, show_LDO_STATUS, store_LDO_STATUS),	\
		.voltage_att = __ATTR(LDO_##_name##_VOLTAGE, 0664, show_LDO_VOLTAGE, store_LDO_VOLTAGE),	\
		.en_reg = (en),	\
		.vol_reg = (vol),	\
		.isUsedable = (use),	\
	}

#define PMIC_BUCK_GEN(_name, en, vol, min, max, step)	\
	{	\
		.desc = {	\
			.name = #_name,	\
			.n_voltages = ((max) - (min)) / (step) + 1,	\
			.min_uV = (min),	\
			.uV_step = (step),	\
		},	\
		.en_att = __ATTR(BUCK_##_name##_STATUS, 0664, show_BUCK_STATUS, store_BUCK_STATUS),	\
		.voltage_att = __ATTR(BUCK_##_name##_VOLTAGE, 0664, show_BUCK_VOLTAGE, store_BUCK_VOLTAGE),	\
		.en_reg = (en),	\
		.vol_reg = (vol),	\
		.isUsedable = 0,	\
	}

*/

#endif				/* _PMIC_SW_H_ */
