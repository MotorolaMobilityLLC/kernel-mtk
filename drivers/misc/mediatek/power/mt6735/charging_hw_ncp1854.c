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

#include <linux/types.h>
#include <mt-plat/charging.h>
#include <mt-plat/upmu_common.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <mt-plat/mt_boot.h>
#include <mt-plat/battery_common.h>
#include <mach/mt_charging.h>
#include <mach/mt_pmic.h>
#include "ncp1854.h"

/* ============================================================ // */
/* Define */
/* ============================================================ // */
#define STATUS_OK	0
#define STATUS_FAIL	1
#define STATUS_UNSUPPORTED	-1
#define GETARRAYNUM(array) (sizeof(array)/sizeof(array[0]))


/* ============================================================ // */
/* Global variable */
/* ============================================================ // */

#if defined(MTK_WIRELESS_CHARGER_SUPPORT)
#define WIRELESS_CHARGER_EXIST_STATE 0

#if defined(GPIO_PWR_AVAIL_WLC)
unsigned int wireless_charger_gpio_number = GPIO_PWR_AVAIL_WLC;
#else
unsigned int wireless_charger_gpio_number = 0;
#endif

#endif

/* As 82 platform mach/charging.h could not cover all voltage setting, just hardcoded below settings */
const unsigned int VBAT_CV_VTH[] = {
	3300000, 3325000, 3350000, 3375000,
	3400000, 3425000, 3450000, 3475000,
	3500000, 3525000, 3550000, 3575000,
	3600000, 3625000, 3650000, 3675000,
	3700000, 3725000, 3750000, 3775000,
	3800000, 3825000, 3850000, 3875000,
	3900000, 3925000, 3950000, 3975000,
	4000000, 4025000, 4050000, 4075000,
	4100000, 4125000, 4150000, 4175000,
	4200000, 4225000, 4250000, 4275000,
	4300000, 4325000, 4350000, 4375000,
	4400000, 4425000, 4450000, 4475000,
};

/* hardcoded current define which defined in NCP1854 IC spec, as common define doesnot cover all define
 * double confirmed with onsemi register set in spec has issue,below is the correct setting */
const unsigned int CS_VTH[] = {
	45000, 50000, 60000, 70000,
	80000, 90000, 100000, 110000,
	120000, 130000, 140000, 150000,
	160000, 170000, 180000, 190000
};

const unsigned int INPUT_CS_VTH[] = {
	CHARGE_CURRENT_100_00_MA, CHARGE_CURRENT_500_00_MA
};

const unsigned int INPUT_CS_VTH_TA[] = {
	CHARGE_CURRENT_600_00_MA, CHARGE_CURRENT_700_00_MA, CHARGE_CURRENT_800_00_MA,
	CHARGE_CURRENT_900_00_MA, CHARGE_CURRENT_1000_00_MA, CHARGE_CURRENT_1100_00_MA,
	CHARGE_CURRENT_1200_00_MA, CHARGE_CURRENT_1300_00_MA, CHARGE_CURRENT_1400_00_MA,
	CHARGE_CURRENT_1500_00_MA, CHARGE_CURRENT_1600_00_MA, 170000,
	180000, 190000, 200000
};

const unsigned int VCDT_HV_VTH[] = {
	BATTERY_VOLT_04_200000_V, BATTERY_VOLT_04_250000_V, BATTERY_VOLT_04_300000_V,
	    BATTERY_VOLT_04_350000_V,
	BATTERY_VOLT_04_400000_V, BATTERY_VOLT_04_450000_V, BATTERY_VOLT_04_500000_V,
	    BATTERY_VOLT_04_550000_V,
	BATTERY_VOLT_04_600000_V, BATTERY_VOLT_06_000000_V, BATTERY_VOLT_06_500000_V,
	    BATTERY_VOLT_07_000000_V,
	BATTERY_VOLT_07_500000_V, BATTERY_VOLT_08_500000_V, BATTERY_VOLT_09_500000_V,
	    BATTERY_VOLT_10_500000_V
};

 /* ============================================================ // */
 /* function prototype */
 /* ============================================================ // */

 /* ============================================================ // */
 /* extern variable */
 /* ============================================================ // */

 /* ============================================================ // */
 /* extern function */
 /* ============================================================ // */
unsigned int current_high_flag = 0;

 /* ============================================================ // */
unsigned int charging_value_to_parameter(const unsigned int *parameter, const unsigned int array_size,
				       const unsigned int val)
{
	unsigned int temp_param;

	if (val < array_size) {
		temp_param = parameter[val];
	} else {
		battery_log(BAT_LOG_CRTI, "Can't find the parameter \r\n");
		temp_param = parameter[0];
	}
	return temp_param;
}

unsigned int charging_parameter_to_value(const unsigned int *parameter, const unsigned int array_size,
				       const unsigned int val)
{
	unsigned int i;

	battery_log(BAT_LOG_FULL, "array_size = %d \r\n", array_size);

	for (i = 0; i < array_size; i++) {
		if (val == *(parameter + i))
			return i;
	}

	battery_log(BAT_LOG_CRTI, "NO register value match. val=%d\r\n", val);

	return 0;
}

static unsigned int bmt_find_closest_level(const unsigned int *pList, unsigned int number,
					 unsigned int level)
{
	unsigned int i, temp_param;
	unsigned int max_value_in_last_element;

	if (pList[0] < pList[1])
		max_value_in_last_element = KAL_TRUE;
	else
		max_value_in_last_element = KAL_FALSE;

	if (max_value_in_last_element == KAL_TRUE) {
		/* max value in the last element */
		for (i = (number - 1); i != 0; i--)	{
			if (pList[i] <= level)
				return pList[i];
		}

		battery_log(BAT_LOG_CRTI, "Can't find closest level, small value first \r\n");
		temp_param = pList[0];
	} else {
		/* max value in the first element */
		for (i = 0; i < number; i++) {
			if (pList[i] <= level)
				return pList[i];
		}

		battery_log(BAT_LOG_CRTI, "Can't find closest level, large value first \r\n");
		temp_param = pList[number - 1];
	}
	return temp_param;
}

static unsigned int charging_hw_init(void *data)
{
	unsigned int ncp1854_status;
	unsigned int status = STATUS_OK;

	pr_notice("[BATTERY:ncp1854] ChargerHwInit_ncp1854\n");

	ncp1854_status = ncp1854_get_chip_status();
	ncp1854_set_trans_en(0);
	ncp1854_set_tj_warn_opt(0x0);	/* set at disabled, by MT6325 BATON */
	/* ncp1854_set_int_mask(0x0); //disable all interrupt */
	ncp1854_set_int_mask(0x1);	/* enable all interrupt for boost mode status monitor */
	/* ncp1854_set_tchg_rst(0x1); //reset charge timer */
	ncp1854_set_chgto_dis(0x1);	/* disable charge timer */
	/* WEAK WAIT, WEAK SAFE, WEAK CHARGE */
	if ((ncp1854_status == 0x8) || (ncp1854_status == 0x9) || (ncp1854_status == 0xA))
		ncp1854_set_ctrl_vbat(0x1C);	/* VCHG = 4.0V */

	ncp1854_set_ieoc(0x0);
	ncp1854_set_iweak(0x3);	/* weak charge current = 300mA */

	ncp1854_set_aicl_en(0x1);	/* enable AICL as PT team suggest */
	ncp1854_set_iinset_pin_en(0x0);	/* Input current limit and AICL control by I2C */
	ncp1854_set_ctrl_vfet(0x3);	/* VFET = 3.4V */

#if defined(MTK_WIRELESS_CHARGER_SUPPORT)
	if (wireless_charger_gpio_number != 0) {
		mt_set_gpio_mode(wireless_charger_gpio_number, 0);	/* 0:GPIO mode */
		mt_set_gpio_dir(wireless_charger_gpio_number, 0);	/* 0: input, 1: output */
	}
#endif

	return status;
}

static unsigned int charging_dump_register(void *data)
{
	unsigned int status = STATUS_OK;

	pr_notice("charging_dump_register\r\n");

	ncp1854_dump_register();

	return status;
}

static unsigned int charging_enable(void *data)
{
	unsigned int status = STATUS_OK;
	unsigned int enable = *(unsigned int *) (data);

	if (KAL_TRUE == enable) {
		ncp1854_set_chg_en(0x1);	/* charger enable */
		/* Set SPM = 1 */
#ifdef GPIO_SWCHARGER_EN_PIN
		mt_set_gpio_mode(GPIO_SWCHARGER_EN_PIN, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO_SWCHARGER_EN_PIN, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_SWCHARGER_EN_PIN, GPIO_OUT_ONE);
#endif
	} else {
#if defined(CONFIG_USB_MTK_HDRC_HCD)
		if (mt_usb_is_device()) {
#endif
			ncp1854_set_chg_en(0x0);	/* charger disable */
#if defined(CONFIG_USB_MTK_HDRC_HCD)
		}
#endif
	}

	return status;
}

static unsigned int charging_set_cv_voltage(void *data)
{
	unsigned int status = STATUS_OK;
	unsigned short register_value;
	unsigned int cv_value = *(unsigned int *) (data);
	unsigned int array_size;
	unsigned int set_chr_cv;

	if (batt_cust_data.high_battery_voltage_support)
		cv_value = BATTERY_VOLT_04_350000_V;

	/* use nearest value */
	array_size = GETARRAYNUM(VBAT_CV_VTH);
	set_chr_cv = bmt_find_closest_level(VBAT_CV_VTH, array_size, cv_value);

	register_value =
	    charging_parameter_to_value(VBAT_CV_VTH, GETARRAYNUM(VBAT_CV_VTH), set_chr_cv);

	ncp1854_set_ctrl_vbat(register_value);

	return status;
}


static unsigned int charging_get_current(void *data)
{
	unsigned int status = STATUS_OK;
	unsigned int array_size;
	unsigned char ret_val = 0;

	/* Get current level */
	/* ret_val = ncp1854_get_ichg(); */
	/* ncp1854_read_interface(NCP1854_CON15, &ret_val, CON15_ICHG_MASK, CON15_ICHG_SHIFT); */
	/* Parsing */
	/* ret_val = (ret_val*100) + 400; */

	array_size = GETARRAYNUM(CS_VTH);
	ret_val = ncp1854_get_ichg();	/* IINLIM */
	if (current_high_flag == 1)
		*(unsigned int *) data =
		    charging_value_to_parameter(CS_VTH, array_size, ret_val) + 160000;
	else
		*(unsigned int *) data = charging_value_to_parameter(CS_VTH, array_size, ret_val);

	return status;
}

static unsigned int charging_set_current(void *data)
{
	unsigned int status = STATUS_OK;
	unsigned int set_chr_current;
	unsigned int array_size;
	unsigned int register_value;
	unsigned int current_value = *(unsigned int *) data;

	array_size = GETARRAYNUM(CS_VTH);
	if (current_value <= 190000) {
		set_chr_current = bmt_find_closest_level(CS_VTH, array_size, current_value);
		register_value = charging_parameter_to_value(CS_VTH, array_size, set_chr_current);
		current_high_flag = 0x0;
	} else {
		set_chr_current =
		    bmt_find_closest_level(CS_VTH, array_size, current_value - 160000);
		register_value = charging_parameter_to_value(CS_VTH, array_size, set_chr_current);
		current_high_flag = 0x1;
	}

	/* current set by SW and disable automatic charge current */
	/* ncp1854_set_aicl_en(0x0); //disable AICL */
	/* set which register first? mmz */
	ncp1854_set_ichg_high(current_high_flag);
	ncp1854_set_ichg(register_value);

	return status;
}

static unsigned int charging_set_input_current(void *data)
{
	unsigned int status = STATUS_OK;
	unsigned int set_chr_current;
	unsigned int array_size;
	unsigned int register_value;
	unsigned int current_value = *(unsigned int *) data;

	if (current_value < 60000) {
		array_size = GETARRAYNUM(INPUT_CS_VTH);
		set_chr_current = bmt_find_closest_level(INPUT_CS_VTH, array_size, current_value);
		register_value =
		    charging_parameter_to_value(INPUT_CS_VTH, array_size, set_chr_current);
		ncp1854_set_iinlim(register_value);
		ncp1854_set_iinlim_ta(0x0);
	} else {
		array_size = GETARRAYNUM(INPUT_CS_VTH_TA);
		set_chr_current =
		    bmt_find_closest_level(INPUT_CS_VTH_TA, array_size, current_value);
		register_value =
		    charging_parameter_to_value(INPUT_CS_VTH_TA, array_size, set_chr_current);
		ncp1854_set_iinlim_ta(register_value);
	}

	ncp1854_set_iinlim_en(0x1);	/* enable input current limit */
	return status;
}

static unsigned int charging_get_charging_status(void *data)
{
	unsigned int status = STATUS_OK;
	unsigned int ret_val;

	ret_val = ncp1854_get_chip_status();
	/* check whether chargeing DONE */
	if (ret_val == 0x6)
		*(unsigned int *) data = KAL_TRUE;
	else
		*(unsigned int *) data = KAL_FALSE;

	return status;
}

void kick_charger_wdt(void)
{
	ncp1854_set_wdto_dis(0x0);
}

static unsigned int charging_reset_watch_dog_timer(void *data)
{
	unsigned int status = STATUS_OK;

	battery_log(BAT_LOG_FULL, "charging_reset_watch_dog_timer\r\n");

	kick_charger_wdt();
	return status;
}

static unsigned int charging_set_hv_threshold(void *data)
{
	unsigned int status = STATUS_OK;
	unsigned int set_hv_voltage;
	unsigned int array_size;
	unsigned short register_value;
	unsigned int voltage = *(unsigned int *) (data);

	array_size = GETARRAYNUM(VCDT_HV_VTH);
	set_hv_voltage = bmt_find_closest_level(VCDT_HV_VTH, array_size, voltage);
	register_value = charging_parameter_to_value(VCDT_HV_VTH, array_size, set_hv_voltage);
	pmic_set_register_value(PMIC_RG_VCDT_HV_VTH, register_value);
	return status;
}

static unsigned int charging_get_hv_status(void *data)
{
	unsigned int status = STATUS_OK;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
	*(kal_bool *) (data) = 0;
	pr_notice("[charging_get_hv_status] charger ok for bring up.\n");
#else
	*(kal_bool *) (data) = pmic_get_register_value(PMIC_RGS_VCDT_HV_DET);
#endif

	return status;
}

static unsigned int charging_get_battery_status(void *data)
{
	unsigned int status = STATUS_OK;
#if !defined(CONFIG_POWER_EXT)
	unsigned int val = 0;
#endif

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
	*(kal_bool *) (data) = 0;	/* battery exist */
	battery_log(BAT_LOG_CRTI, "[charging_get_battery_status] battery exist for bring up.\n");
#else
	val = pmic_get_register_value(PMIC_BATON_TDET_EN);
	battery_log(BAT_LOG_FULL, "[charging_get_battery_status] BATON_TDET_EN = %d\n", val);
	if (val) {
		pmic_set_register_value(PMIC_BATON_TDET_EN, 1);
		pmic_set_register_value(PMIC_RG_BATON_EN, 1);
		*(kal_bool *) (data) = pmic_get_register_value(PMIC_RGS_BATON_UNDET);
	} else {
		*(kal_bool *) (data) = KAL_FALSE;
	}
#endif

	return status;
}

static unsigned int charging_get_charger_det_status(void *data)
{
	unsigned int status = STATUS_OK;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
	*(kal_bool *) (data) = 1;
	battery_log(BAT_LOG_CRTI, "[charging_get_charger_det_status] chr exist for fpga.\n");
#else
	*(kal_bool *) (data) = pmic_get_register_value_nolock(PMIC_RGS_CHRDET);
#endif

	return status;
}

static unsigned int charging_get_charger_type(void *data)
{
	unsigned int status = STATUS_OK;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
	*(CHARGER_TYPE *) (data) = STANDARD_HOST;
#else
	*(CHARGER_TYPE *) (data) = hw_charging_get_charger_type();
#endif

	return status;
}

static unsigned int charging_set_platform_reset(void *data)
{
	unsigned int status = STATUS_OK;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
#else
	battery_log(BAT_LOG_CRTI, "charging_set_platform_reset\n");

	kernel_restart("battery service reboot system");
	/* arch_reset(0,NULL); */
#endif

	return status;
}

static unsigned int charging_get_platform_boot_mode(void *data)
{
	unsigned int status = STATUS_OK;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
#else
	*(unsigned int *) (data) = get_boot_mode();

	battery_log(BAT_LOG_CRTI, "get_boot_mode=%d\n", get_boot_mode());
#endif

	return status;
}

static unsigned int charging_set_power_off(void *data)
{
	unsigned int status = STATUS_OK;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
#else
	battery_log(BAT_LOG_CRTI, "charging_set_power_off\n");
	kernel_power_off();
#endif

	return status;
}

static unsigned int charging_set_ta_current_pattern(void *data)
{
	unsigned int status = STATUS_OK;
	unsigned int increase = *(unsigned int *) (data);

#if defined(HIGH_BATTERY_VOLTAGE_SUPPORT)
	BATTERY_VOLTAGE_ENUM cv_voltage = BATTERY_VOLT_04_350000_V;
#else
	BATTERY_VOLTAGE_ENUM cv_voltage = BATTERY_VOLT_04_200000_V;
#endif

	charging_set_cv_voltage(&cv_voltage);	/* Set CV */
	ncp1854_set_ichg(0x01);	/* Set charging current 500ma */
	ncp1854_set_ichg_high(0x00);

	ncp1854_set_iinlim_ta(0x00);
	ncp1854_set_iinlim(0x01);	/* set charging iinlim 500ma */

	ncp1854_set_iinlim_en(0x01);	/* enable iinlim */
	ncp1854_set_chg_en(0x01);	/* Enable Charging */

	/* ncp1854_dump_register(); */

	if (increase == KAL_TRUE) {
		ncp1854_set_iinlim(0x0);	/* 100mA */
		msleep(85);

		ncp1854_set_iinlim(0x1);	/* 500mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_increase() on 1");
		msleep(85);

		ncp1854_set_iinlim(0x0);	/* 100mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_increase() off 1");
		msleep(85);

		ncp1854_set_iinlim(0x1);	/* 500mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_increase() on 2");
		msleep(85);

		ncp1854_set_iinlim(0x0);	/* 100mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_increase() off 2");
		msleep(85);

		ncp1854_set_iinlim(0x1);	/* 500mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_increase() on 3");
		msleep(281);

		ncp1854_set_iinlim(0x0);	/* 100mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_increase() off 3");
		msleep(85);

		ncp1854_set_iinlim(0x1);	/* 500mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_increase() on 4");
		msleep(281);

		ncp1854_set_iinlim(0x0);	/* 100mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_increase() off 4");
		msleep(85);

		ncp1854_set_iinlim(0x1);	/* 500mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_increase() on 5");
		msleep(281);

		ncp1854_set_iinlim(0x0);	/* 100mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_increase() off 5");
		msleep(85);

		ncp1854_set_iinlim(0x1);	/* 500mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_increase() on 6");
		msleep(485);

		ncp1854_set_iinlim(0x0);	/* 100mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_increase() off 6");
		msleep(50);

		battery_log(BAT_LOG_CRTI, "mtk_ta_increase() end\n");

		ncp1854_set_iinlim(0x1);	/* 500mA */
		msleep(200);
	} else {
		ncp1854_set_iinlim(0x0);	/* 100mA */
		msleep(85);

		ncp1854_set_iinlim(0x1);	/* 500mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_decrease() on 1");
		msleep(281);

		ncp1854_set_iinlim(0x0);	/* 100mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_decrease() off 1");
		msleep(85);

		ncp1854_set_iinlim(0x1);	/* 500mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_decrease() on 2");
		msleep(281);

		ncp1854_set_iinlim(0x0);	/* 100mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_decrease() off 2");
		msleep(85);

		ncp1854_set_iinlim(0x1);	/* 500mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_decrease() on 3");
		msleep(281);

		ncp1854_set_iinlim(0x0);	/* 100mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_decrease() off 3");
		msleep(85);

		ncp1854_set_iinlim(0x1);	/* 500mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_decrease() on 4");
		msleep(85);

		ncp1854_set_iinlim(0x0);	/* 100mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_decrease() off 4");
		msleep(85);

		ncp1854_set_iinlim(0x1);	/* 500mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_decrease() on 5");
		msleep(85);

		ncp1854_set_iinlim(0x0);	/* 100mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_decrease() off 5");
		msleep(85);

		ncp1854_set_iinlim(0x1);	/* 500mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_decrease() on 6");
		msleep(485);

		ncp1854_set_iinlim(0x0);	/* 100mA */
		battery_log(BAT_LOG_FULL, "mtk_ta_decrease() off 6");
		msleep(50);

		battery_log(BAT_LOG_CRTI, "mtk_ta_decrease() end\n");

		ncp1854_set_iinlim(0x1);	/* 500mA */
	}

	return status;
}

static unsigned int charging_set_vbus_ovp_en(void *data)
{
	return STATUS_OK;
}

static unsigned int charging_set_vindpm(void *data)
{
	return STATUS_OK;
}

static unsigned int(*charging_func[CHARGING_CMD_NUMBER]) (void *data);

/*
* FUNCTION
*		Internal_chr_control_handler
*
* DESCRIPTION
*		 This function is called to set the charger hw
*
* CALLS
*
* PARAMETERS
*		None
*
* RETURNS
*
*
* GLOBALS AFFECTED
*	   None
*/
signed int chr_control_interface(CHARGING_CTRL_CMD cmd, void *data)
{
	static signed int init = -1;

	if (init == -1) {
		init = 0;
		charging_func[CHARGING_CMD_INIT] = charging_hw_init;
		charging_func[CHARGING_CMD_DUMP_REGISTER] = charging_dump_register;
		charging_func[CHARGING_CMD_ENABLE] = charging_enable;
		charging_func[CHARGING_CMD_SET_CV_VOLTAGE] = charging_set_cv_voltage;
		charging_func[CHARGING_CMD_GET_CURRENT] = charging_get_current;
		charging_func[CHARGING_CMD_SET_CURRENT] = charging_set_current;
		charging_func[CHARGING_CMD_SET_INPUT_CURRENT] = charging_set_input_current;
		charging_func[CHARGING_CMD_GET_CHARGING_STATUS] =  charging_get_charging_status;
		charging_func[CHARGING_CMD_RESET_WATCH_DOG_TIMER] = charging_reset_watch_dog_timer;
		charging_func[CHARGING_CMD_SET_HV_THRESHOLD] = charging_set_hv_threshold;
		charging_func[CHARGING_CMD_GET_HV_STATUS] = charging_get_hv_status;
		charging_func[CHARGING_CMD_GET_BATTERY_STATUS] = charging_get_battery_status;
		charging_func[CHARGING_CMD_GET_CHARGER_DET_STATUS] = charging_get_charger_det_status;
		charging_func[CHARGING_CMD_GET_CHARGER_TYPE] = charging_get_charger_type;
		charging_func[CHARGING_CMD_SET_PLATFORM_RESET] = charging_set_platform_reset;
		charging_func[CHARGING_CMD_GET_PLATFORM_BOOT_MODE] = charging_get_platform_boot_mode;
		charging_func[CHARGING_CMD_SET_POWER_OFF] = charging_set_power_off;
		charging_func[CHARGING_CMD_SET_TA_CURRENT_PATTERN] = charging_set_ta_current_pattern;
		charging_func[CHARGING_CMD_SET_VBUS_OVP_EN] = charging_set_vbus_ovp_en;
		charging_func[CHARGING_CMD_SET_VINDPM] = charging_set_vindpm;
	}

	if (cmd < CHARGING_CMD_NUMBER) {
		if (charging_func[cmd] != NULL)
			return charging_func[cmd](data);
	}

	pr_debug("[%s]UNSUPPORT Function: %d\n", __func__, cmd);

	return STATUS_UNSUPPORTED;
}
