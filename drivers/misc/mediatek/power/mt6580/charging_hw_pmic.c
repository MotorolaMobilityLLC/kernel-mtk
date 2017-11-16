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
#include <mt-plat/battery_meter.h>
#include <mach/mt_battery_meter.h>
#include <mach/mt_charging.h>
#include <mach/mt_pmic.h>




/* ============================================================ */
/*define*/
/* ============================================================ */
#define STATUS_OK    0
#define STATUS_FAIL	1
#define STATUS_UNSUPPORTED    -1
#define GETARRAYNUM(array) (sizeof(array)/sizeof(array[0]))


/* ============================================================ */
/*global variable*/
/* ============================================================ */
kal_bool chargin_hw_init_done = KAL_TRUE;
kal_bool charging_type_det_done = KAL_TRUE;

const unsigned int VBAT_CV_VTH[] = {
	BATTERY_VOLT_04_200000_V, BATTERY_VOLT_04_212500_V, BATTERY_VOLT_04_225000_V,
	BATTERY_VOLT_04_237500_V,
	BATTERY_VOLT_04_250000_V, BATTERY_VOLT_04_262500_V, BATTERY_VOLT_04_275000_V,
	BATTERY_VOLT_04_300000_V,
	BATTERY_VOLT_04_325000_V, BATTERY_VOLT_04_350000_V, BATTERY_VOLT_04_375000_V,
	BATTERY_VOLT_04_400000_V,
	BATTERY_VOLT_04_425000_V, BATTERY_VOLT_04_162500_V, BATTERY_VOLT_04_175000_V,
	BATTERY_VOLT_02_200000_V,
	BATTERY_VOLT_04_050000_V, BATTERY_VOLT_04_100000_V, BATTERY_VOLT_04_125000_V,
	BATTERY_VOLT_03_775000_V,
	BATTERY_VOLT_03_800000_V, BATTERY_VOLT_03_850000_V, BATTERY_VOLT_03_900000_V,
	BATTERY_VOLT_04_000000_V,
	BATTERY_VOLT_04_050000_V, BATTERY_VOLT_04_100000_V, BATTERY_VOLT_04_125000_V,
	BATTERY_VOLT_04_137500_V,
	BATTERY_VOLT_04_150000_V, BATTERY_VOLT_04_162500_V, BATTERY_VOLT_04_175000_V,
	BATTERY_VOLT_04_187500_V
};

const unsigned int CS_VTH[] = {
	CHARGE_CURRENT_1600_00_MA, CHARGE_CURRENT_1500_00_MA, CHARGE_CURRENT_1400_00_MA,
	CHARGE_CURRENT_1300_00_MA,
	CHARGE_CURRENT_1200_00_MA, CHARGE_CURRENT_1100_00_MA, CHARGE_CURRENT_1000_00_MA,
	CHARGE_CURRENT_900_00_MA,
	CHARGE_CURRENT_800_00_MA, CHARGE_CURRENT_700_00_MA, CHARGE_CURRENT_650_00_MA,
	CHARGE_CURRENT_550_00_MA,
	CHARGE_CURRENT_450_00_MA, CHARGE_CURRENT_300_00_MA, CHARGE_CURRENT_200_00_MA,
	CHARGE_CURRENT_70_00_MA
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


/* ============================================================ */

static unsigned int charging_get_csdac_value(void)
{
	unsigned tempA, tempB, tempC;
	unsigned sum;

	pmic_config_interface(MT6350_CHR_CON11, 0xC, 0xF, 0);
	pmic_read_interface(MT6350_CHR_CON10, &tempC, 0xF, 0x0);	/* bit 1 and 2 mapping bit 8 and bit9 */

	pmic_config_interface(MT6350_CHR_CON11, 0xA, 0xF, 0);
	pmic_read_interface(MT6350_CHR_CON10, &tempA, 0xF, 0x0);	/* bit 0 ~ 3 mapping bit 4 ~7 */

	pmic_config_interface(MT6350_CHR_CON11, 0xB, 0xF, 0);
	pmic_read_interface(MT6350_CHR_CON10, &tempB, 0xF, 0x0);	/* bit 0~3 mapping bit 0~3 */

	sum = (((tempC & 0x6) >> 1) << 8) | (tempA << 4) | tempB;

	battery_xlog_printk(BAT_LOG_CRTI, "tempC=%d,tempA=%d,tempB=%d, csdac=%d\n", tempC, tempA,
			    tempB, sum);

	return sum;
}


unsigned int charging_value_to_parameter(const unsigned int *parameter,
					 const unsigned int array_size, const unsigned int val)
{
	if (val < array_size)
		return parameter[val];


	battery_log(BAT_LOG_CRTI, "Can't find the parameter \r\n");
	return parameter[0];

}


unsigned int charging_parameter_to_value(const unsigned int *parameter,
					 const unsigned int array_size, const unsigned int val)
{
	unsigned int i;

	for (i = 0; i < array_size; i++) {
		if (val == *(parameter + i))
			return i;
	}

	battery_log(BAT_LOG_CRTI, "NO register value match \r\n");
	/*TODO: ASSERT(0);    // not find the value */
	return 0;
}


static unsigned int bmt_find_closest_level(const unsigned int *pList, unsigned int number,
					   unsigned int level)
{
	unsigned int i;
	unsigned int max_value_in_last_element;

	if (pList[0] < pList[1])
		max_value_in_last_element = KAL_TRUE;
	else
		max_value_in_last_element = KAL_FALSE;

	if (max_value_in_last_element == KAL_TRUE) {
		for (i = (number - 1); i != 0; i--) {
			if (pList[i] <= level)
				return pList[i];
		}

		battery_log(BAT_LOG_CRTI, "Can't find closest level \r\n");
		return pList[0];
		/*return CHARGE_CURRENT_0_00_MA; */
	} else {
		for (i = 0; i < number; i++) {
			if (pList[i] <= level)
				return pList[i];
		}

		battery_log(BAT_LOG_CRTI, "Can't find closest level \r\n");
		return pList[number - 1];
		/*return CHARGE_CURRENT_0_00_MA; */
	}
}

static unsigned int charging_hw_init(void *data)
{
	unsigned int status = STATUS_OK;

	pmic_set_register_value(PMIC_RG_CHRWDT_TD, 0);	/* CHRWDT_TD, 4s */
	pmic_set_register_value(PMIC_RG_CHRWDT_INT_EN, 1);	/* CHRWDT_INT_EN */
	pmic_set_register_value(PMIC_RG_CHRWDT_EN, 1);	/* CHRWDT_EN */
	pmic_set_register_value(PMIC_RG_CHRWDT_WR, 1);	/* CHRWDT_WR */

	pmic_set_register_value(PMIC_RG_VCDT_MODE, 0);	/*VCDT_MODE */
	pmic_set_register_value(PMIC_RG_VCDT_HV_EN, 1);	/*VCDT_HV_EN */

	pmic_set_register_value(PMIC_RG_USBDL_SET, 0);	/*force leave USBDL mode */
	pmic_set_register_value(PMIC_RG_USBDL_RST, 1);	/*force leave USBDL mode */

	pmic_set_register_value(PMIC_RG_BC11_BB_CTRL, 1);	/*BC11_BB_CTRL */
	pmic_set_register_value(PMIC_RG_BC11_RST, 1);	/*BC11_RST */

	pmic_set_register_value(PMIC_RG_CSDAC_MODE, 1);	/*CSDAC_MODE */
	pmic_set_register_value(PMIC_RG_VBAT_OV_EN, 1);	/*VBAT_OV_EN */

#ifdef HIGH_BATTERY_VOLTAGE_SUPPORT
	pmic_set_register_value(PMIC_RG_VBAT_OV_VTH, 2);	/*VBAT_OV_VTH, 4.4V, */
#else
	pmic_set_register_value(PMIC_RG_VBAT_OV_VTH, 1);	/*VBAT_OV_VTH, 4.3V, */
#endif
	pmic_set_register_value(PMIC_RG_BATON_EN, 1);	/*BATON_EN */

	/*Tim, for TBAT */
	pmic_set_register_value(PMIC_RG_BATON_HT_EN, 0);	/*BATON_HT_EN */

	pmic_set_register_value(PMIC_RG_ULC_DET_EN, 1);	/* RG_ULC_DET_EN=1 */
	pmic_set_register_value(PMIC_RG_LOW_ICH_DB, 1);	/* RG_LOW_ICH_DB=000001'b */

#if defined(CONFIG_MTK_PUMP_EXPRESS_SUPPORT)
	pmic_set_register_value(PMIC_RG_CSDAC_DLY, 0);	/* CSDAC_DLY */
	pmic_set_register_value(PMIC_RG_CSDAC_STP, 1);	/* CSDAC_STP */
	pmic_set_register_value(PMIC_RG_CSDAC_STP_INC, 1);	/* CSDAC_STP_INC */
	pmic_set_register_value(PMIC_RG_CSDAC_STP_DEC, 7);	/* CSDAC_STP_DEC */
	pmic_set_register_value(PMIC_RG_CS_EN, 1);	/* CS_EN */

	pmic_set_register_value(PMIC_RG_HWCV_EN, 1);
	pmic_set_register_value(PMIC_RG_VBAT_CV_EN, 1);	/* CV_EN */
	pmic_set_register_value(PMIC_RG_CSDAC_EN, 1);	/* CSDAC_EN */
	pmic_set_register_value(PMIC_RG_CHR_EN, 1);	/* CHR_EN */
#endif

	return status;
}


static unsigned int charging_dump_register(void *data)
{
	unsigned int status = STATUS_OK;
	unsigned int i = 0;

	if (Enable_BATDRV_LOG >= BAT_LOG_FULL) {
		for (i = MT6350_CHR_CON0; i <= MT6350_CHR_CON29; i += 10) {
			battery_log(BAT_LOG_CRTI,
				    "[0x%x]=0x%x,[0x%x]=0x%x,[0x%x]=0x%x,[0x%x]=0x%x,[0x%x]=0x%x\n",
				    i, upmu_get_reg_value(i),
				    i + 2, upmu_get_reg_value(i + 2),
				    i + 4, upmu_get_reg_value(i + 4),
				    i + 6, upmu_get_reg_value(i + 6),
				    i + 8, upmu_get_reg_value(i + 8));
		}
	}

	return status;
}


static unsigned int charging_enable(void *data)
{
	unsigned int status = STATUS_OK;
	unsigned int enable = *(unsigned int *)(data);

	if (KAL_TRUE == enable) {
		pmic_set_register_value(PMIC_RG_CSDAC_DLY, 4);	/* CSDAC_DLY */
		pmic_set_register_value(PMIC_RG_CSDAC_STP, 1);	/* CSDAC_STP */
		pmic_set_register_value(PMIC_RG_CSDAC_STP_INC, 1);	/* CSDAC_STP_INC */
		pmic_set_register_value(PMIC_RG_CSDAC_STP_DEC, 2);	/* CSDAC_STP_DEC */
		pmic_set_register_value(PMIC_RG_CS_EN, 1);	/* CS_EN, check me */

		pmic_set_register_value(PMIC_RG_HWCV_EN, 1);

		pmic_set_register_value(PMIC_RG_VBAT_CV_EN, 1);	/* CV_EN */
		pmic_set_register_value(PMIC_RG_CSDAC_EN, 1);	/* CSDAC_EN        */

		pmic_set_register_value(PMIC_RG_PCHR_FLAG_EN, 1);	/* enable debug falg output */

		pmic_set_register_value(PMIC_RG_CHR_EN, 1);	/* CHR_EN */
	} else {
		pmic_set_register_value(PMIC_RG_CHRWDT_INT_EN, 0);	/* CHRWDT_INT_EN */
		pmic_set_register_value(PMIC_RG_CHRWDT_EN, 0);	/* CHRWDT_EN */
		pmic_set_register_value(PMIC_RG_CHRWDT_FLAG_WR, 0);	/* CHRWDT_FLAG */
		pmic_set_register_value(PMIC_RG_CSDAC_EN, 0);	/* CSDAC_EN */
		pmic_set_register_value(PMIC_RG_CHR_EN, 0);	/* CHR_EN */
		pmic_set_register_value(PMIC_RG_HWCV_EN, 0);	/* RG_HWCV_EN */
	}
	return status;
}




static unsigned int charging_set_cv_voltage(void *data)
{
	unsigned int status = STATUS_OK;
	unsigned short register_value;

	register_value =
	    charging_parameter_to_value(VBAT_CV_VTH, GETARRAYNUM(VBAT_CV_VTH),
					*(unsigned int *)(data));

	pmic_set_register_value(PMIC_RG_VBAT_CV_VTH, register_value);

	battery_log(BAT_LOG_CRTI, "[charging_set_cv_voltage] [0x%x]=0x%x, [0x%x]=0x%x\n",
		    0x000c, upmu_get_reg_value(0x000c), 0x0006, upmu_get_reg_value(0x0006));

	return status;
}


static unsigned int charging_get_current(void *data)
{
	unsigned int status = STATUS_OK;
	unsigned int array_size;
	unsigned int reg_value;

	array_size = GETARRAYNUM(CS_VTH);
	reg_value = pmic_get_register_value(PMIC_RG_CS_VTH);	/*RG_CS_VTH */
	*(unsigned int *)data = charging_value_to_parameter(CS_VTH, array_size, reg_value);

	return status;
}


static unsigned int charging_set_current(void *data)
{
	unsigned int status = STATUS_OK;
	unsigned int set_chr_current;
	unsigned int array_size;
	unsigned int register_value;

	array_size = GETARRAYNUM(CS_VTH);
	set_chr_current = bmt_find_closest_level(CS_VTH, array_size, *(unsigned int *)data);
	register_value = charging_parameter_to_value(CS_VTH, array_size, set_chr_current);
	pmic_set_register_value(PMIC_RG_CS_VTH, register_value);

	return status;
}


static unsigned int charging_set_input_current(void *data)
{
	unsigned int status = STATUS_OK;
	return status;
}

static unsigned int charging_get_charging_status(void *data)
{
	unsigned int status = STATUS_OK;
	return status;
}


static unsigned int charging_reset_watch_dog_timer(void *data)
{
	unsigned int status = STATUS_OK;

	pmic_set_register_value(PMIC_RG_CHRWDT_TD, 0);	/* CHRWDT_TD, 4s */
	pmic_set_register_value(PMIC_RG_CHRWDT_WR, 1);	/* CHRWDT_WR */
	pmic_set_register_value(PMIC_RG_CHRWDT_INT_EN, 1);	/* CHRWDT_INT_EN */
	pmic_set_register_value(PMIC_RG_CHRWDT_EN, 1);	/* CHRWDT_EN */
	pmic_set_register_value(PMIC_RG_CHRWDT_FLAG_WR, 1);	/* CHRWDT_WR */

	return status;
}


static unsigned int charging_set_hv_threshold(void *data)
{
	unsigned int status = STATUS_OK;

	unsigned int set_hv_voltage;
	unsigned int array_size;
	unsigned short register_value;
	unsigned int voltage = *(unsigned int *)(data);

	array_size = GETARRAYNUM(VCDT_HV_VTH);
	set_hv_voltage = bmt_find_closest_level(VCDT_HV_VTH, array_size, voltage);
	register_value = charging_parameter_to_value(VCDT_HV_VTH, array_size, set_hv_voltage);
	pmic_set_register_value(PMIC_RG_VCDT_HV_VTH, register_value);

	return status;
}


static unsigned int charging_get_hv_status(void *data)
{
	unsigned int status = STATUS_OK;

	*(kal_bool *) (data) = pmic_get_register_value(PMIC_RGS_VCDT_HV_DET);

	return status;
}


static unsigned int charging_get_battery_status(void *data)
{
	unsigned int status = STATUS_OK;
#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
	*(kal_bool *) (data) = 0;	/* battery exist */
	battery_log(BAT_LOG_CRTI, "bat exist for evb\n");
#else
	unsigned int val = 0;

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

#if defined(CONFIG_MTK_FPGA)
	*(kal_bool *) (data) = 1;
	battery_log(BAT_LOG_CRTI, "chr exist for fpga!\n");
#else
	*(kal_bool *) (data) = pmic_get_register_value(PMIC_RGS_CHRDET);
#endif

	return status;
}


kal_bool charging_type_detection_done(void)
{
	return charging_type_det_done;
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

static unsigned int charging_get_is_pcm_timer_trigger(void *data)
{
	unsigned int status = STATUS_OK;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
	*(kal_bool *) (data) = KAL_FALSE;
#else

/*     if (slp_get_wake_reason() == WR_PCM_TIMER)*/
/*        *(kal_bool*)(data) = KAL_TRUE;*/
/*     else*/
	*(kal_bool *) (data) = KAL_FALSE;

/*     battery_log(BAT_LOG_CRTI, "slp_get_wake_reason=%d\n", slp_get_wake_reason());*/

#endif

	return status;
}

static unsigned int charging_set_platform_reset(void *data)
{
	unsigned int status = STATUS_OK;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
#else
	battery_log(BAT_LOG_CRTI, "charging_set_platform_reset\n");

/*	arch_reset(0, NULL);*/
#endif

	return status;
}

static unsigned int charging_get_platform_boot_mode(void *data)
{
	unsigned int status = STATUS_OK;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
#else
	*(unsigned int *)(data) = get_boot_mode();

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

static unsigned int charging_get_power_source(void *data)
{
	unsigned int status = STATUS_OK;

#if 0				/*#if defined(MTK_POWER_EXT_DETECT) */
	if (MT_BOARD_PHONE == mt_get_board_type())
		*(kal_bool *) data = KAL_FALSE;
	else
		*(kal_bool *) data = KAL_TRUE;
#else
	*(kal_bool *) data = KAL_FALSE;
#endif

	return status;
}

static unsigned int charging_get_csdac_full_flag(void *data)
{
	unsigned int status = STATUS_OK;
	unsigned int csdac_value;

	csdac_value = charging_get_csdac_value();

	if (csdac_value > 800)
		*(kal_bool *) data = KAL_TRUE;
	else
		*(kal_bool *) data = KAL_FALSE;

	return status;

}

static unsigned int charging_set_ta_current_pattern(void *data)
{
	unsigned int status = STATUS_OK;
	unsigned int array_size;
	unsigned int ta_on_current = CHARGE_CURRENT_450_00_MA;
	unsigned int ta_off_current = CHARGE_CURRENT_70_00_MA;
	unsigned int set_ta_on_current_reg_value;
	unsigned int set_ta_off_current_reg_value;
	unsigned int increase = *(unsigned int *)(data);

	array_size = GETARRAYNUM(CS_VTH);
	set_ta_on_current_reg_value =
	    charging_parameter_to_value(CS_VTH, array_size, ta_on_current);
	set_ta_off_current_reg_value =
	    charging_parameter_to_value(CS_VTH, array_size, ta_off_current);

	if (increase == KAL_TRUE) {
		battery_log(BAT_LOG_CRTI, "mtk_ta_increase() start\n");

		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_off_current_reg_value);
		msleep(50);

		/* patent start */
		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_on_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_increase() on 1\n");
		msleep(100);
		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_off_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_increase() off 1\n");
		msleep(100);


		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_on_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_increase() on 2\n");
		msleep(100);
		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_off_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_increase() off 2\n");
		msleep(100);


		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_on_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_increase() on 3\n");
		msleep(300);
		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_off_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_increase() off 3\n");
		msleep(100);


		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_on_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_increase() on 4\n");
		msleep(300);
		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_off_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_increase() off 4\n");
		msleep(100);


		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_on_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_increase() on 5\n");
		msleep(300);
		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_off_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_increase() off 5\n");
		msleep(100);


		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_on_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_increase() on 6\n");
		msleep(500);

		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_off_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_increase() off 6\n");
		msleep(50);
		/* patent end */

		battery_log(BAT_LOG_CRTI, "mtk_ta_increase() end\n");
	} else {
		battery_log(BAT_LOG_CRTI, "mtk_ta_decrease() start\n");

		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_off_current_reg_value);
		msleep(50);

		/* patent start    */
		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_on_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_decrease() on 1");
		msleep(300);
		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_off_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_decrease() off 1");
		msleep(100);


		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_on_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_decrease() on 2");
		msleep(300);
		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_off_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_decrease() off 2");
		msleep(100);


		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_on_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_decrease() on 3");
		msleep(300);
		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_off_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_decrease() off 3");
		msleep(100);


		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_on_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_decrease() on 4");
		msleep(100);
		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_off_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_decrease() off 4");
		msleep(100);


		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_on_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_decrease() on 5");
		msleep(100);
		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_off_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_decrease() off 5");
		msleep(100);


		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_on_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_decrease() on 6");
		msleep(500);

		pmic_set_register_value(PMIC_RG_CS_VTH, set_ta_off_current_reg_value);
		battery_log(BAT_LOG_CRTI, "mtk_ta_decrease() off 6");
		msleep(50);
		/* patent end */

		battery_log(BAT_LOG_CRTI, "mtk_ta_decrease() end\n");
	}

	return status;
}


static unsigned int charging_set_error_state(void *data)
{
	return STATUS_UNSUPPORTED;
}

static unsigned int (*const charging_func[CHARGING_CMD_NUMBER]) (void *data) = {
charging_hw_init, charging_dump_register, charging_enable, charging_set_cv_voltage,
	    charging_get_current, charging_set_current, charging_set_input_current,
	    charging_get_charging_status, charging_reset_watch_dog_timer,
	    charging_set_hv_threshold, charging_get_hv_status, charging_get_battery_status,
	    charging_get_charger_det_status, charging_get_charger_type,
	    charging_get_is_pcm_timer_trigger, charging_set_platform_reset,
	    charging_get_platform_boot_mode, charging_set_power_off,
	    charging_get_power_source, charging_get_csdac_full_flag,
	    charging_set_ta_current_pattern, charging_set_error_state};


/*
* FUNCTION
*        Internal_chr_control_handler
*
* DESCRIPTION
*         This function is called to set the charger hw
*
* CALLS
*
* PARAMETERS
*        None
*
* RETURNS
*
*
* GLOBALS AFFECTED
*       None
*/
signed int chr_control_interface(CHARGING_CTRL_CMD cmd, void *data)
{
	signed int status;

	if (cmd < CHARGING_CMD_NUMBER) {
		if (charging_func[cmd] != NULL)
			status = charging_func[cmd](data);
		else {
			battery_log(BAT_LOG_CRTI, "[chr_control_interface]cmd:%d not supported\n", cmd);
			status = STATUS_UNSUPPORTED;
		}
	} else
		status = STATUS_UNSUPPORTED;

	return status;
}
