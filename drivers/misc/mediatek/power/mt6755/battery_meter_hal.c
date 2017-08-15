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


#include <linux/delay.h>
#include <asm/div64.h>

#include <linux/mutex.h>
#include <mt-plat/upmu_common.h>

#include <mt-plat/battery_meter_hal.h>
#include <mach/mt_battery_meter.h>
#include <mach/mt_pmic.h>
#include <mt-plat/battery_meter.h>

static DEFINE_MUTEX(FGADC_hal_mutex);

void fgadc_hal_lock(void)
{
	mutex_lock(&FGADC_hal_mutex);
}

void fgadc_hal_unlock(void)
{
	mutex_unlock(&FGADC_hal_mutex);
}

/*============================================================ //
//define
//============================================================ //*/
#define STATUS_OK    0
#define STATUS_UNSUPPORTED    -1
#define VOLTAGE_FULL_RANGE    1800
#define ADC_PRECISE           32768	/* 12 bits */

#define UNIT_FGCURRENT     (158122)	/* 158.122 uA */

/*============================================================ //
//global variable
//============================================================ //*/
signed int chip_diff_trim_value_4_0 = 0;
signed int chip_diff_trim_value = 0;	/* unit = 0.1 */

signed int g_hw_ocv_tune_value = 0;

kal_bool g_fg_is_charging = 0;

signed int g_meta_input_cali_current;

/*============================================================ //
//function prototype
//============================================================ //

//============================================================ //
//extern variable
//============================================================ //

//============================================================ //
//extern function
//============================================================ //*/

/*============================================================ // */
void get_hw_chip_diff_trim_value(void)
{
#if defined(CONFIG_POWER_EXT)
#else

#if 1

	chip_diff_trim_value_4_0 = 0;

	bm_print(BM_LOG_CRTI, "[Chip_Trim] chip_diff_trim_value_4_0=%d\n",
		 chip_diff_trim_value_4_0);
#else
	bm_print(BM_LOG_FULL, "[Chip_Trim] need check reg number\n");
#endif

	switch (chip_diff_trim_value_4_0) {
	case 0:
		chip_diff_trim_value = 1000;
		break;
	case 1:
		chip_diff_trim_value = 1005;
		break;
	case 2:
		chip_diff_trim_value = 1010;
		break;
	case 3:
		chip_diff_trim_value = 1015;
		break;
	case 4:
		chip_diff_trim_value = 1020;
		break;
	case 5:
		chip_diff_trim_value = 1025;
		break;
	case 6:
		chip_diff_trim_value = 1030;
		break;
	case 7:
		chip_diff_trim_value = 1036;
		break;
	case 8:
		chip_diff_trim_value = 1041;
		break;
	case 9:
		chip_diff_trim_value = 1047;
		break;
	case 10:
		chip_diff_trim_value = 1052;
		break;
	case 11:
		chip_diff_trim_value = 1058;
		break;
	case 12:
		chip_diff_trim_value = 1063;
		break;
	case 13:
		chip_diff_trim_value = 1069;
		break;
	case 14:
		chip_diff_trim_value = 1075;
		break;
	case 15:
		chip_diff_trim_value = 1081;
		break;
	case 31:
		chip_diff_trim_value = 995;
		break;
	case 30:
		chip_diff_trim_value = 990;
		break;
	case 29:
		chip_diff_trim_value = 985;
		break;
	case 28:
		chip_diff_trim_value = 980;
		break;
	case 27:
		chip_diff_trim_value = 975;
		break;
	case 26:
		chip_diff_trim_value = 970;
		break;
	case 25:
		chip_diff_trim_value = 966;
		break;
	case 24:
		chip_diff_trim_value = 961;
		break;
	case 23:
		chip_diff_trim_value = 956;
		break;
	case 22:
		chip_diff_trim_value = 952;
		break;
	case 21:
		chip_diff_trim_value = 947;
		break;
	case 20:
		chip_diff_trim_value = 943;
		break;
	case 19:
		chip_diff_trim_value = 938;
		break;
	case 18:
		chip_diff_trim_value = 934;
		break;
	case 17:
		chip_diff_trim_value = 930;
		break;
	default:
		bm_print(BM_LOG_FULL, "[Chip_Trim] Invalid value(%d)\n",
			 chip_diff_trim_value_4_0);
		break;
	}

	bm_print(BM_LOG_CRTI, "[Chip_Trim] chip_diff_trim_value=%d\n", chip_diff_trim_value);
#endif
}

signed int use_chip_trim_value(signed int not_trim_val)
{
#if defined(CONFIG_POWER_EXT)
	return not_trim_val;
#else
	signed int ret_val = 0;

	ret_val = ((not_trim_val * chip_diff_trim_value) / 1000);

	bm_print(BM_LOG_FULL, "[use_chip_trim_value] %d -> %d\n", not_trim_val, ret_val);

	return ret_val;
#endif
}

int get_hw_ocv(void)
{
#if defined(CONFIG_POWER_EXT)
	return 4001;
	bm_print(BM_LOG_CRTI, "[get_hw_ocv] TBD\n");
#else
	signed int adc_result_reg = 0;
	signed int adc_result = 0;
	signed int r_val_temp = 3;           /*MT6325 use 2, old chip use 4    */
	int reg_sel;
#if defined(SWCHR_POWER_PATH)
	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		adc_result_reg = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_WAKEUP_SWCHR);
		reg_sel = pmic_get_register_value(PMIC_RG_STRUP_AUXADC_START_SEL);
	#else
		adc_result_reg = pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_OUT_WAKEUP_SWCHR);
		reg_sel = pmic_get_register_value(MT6351_PMIC_STRUP_AUXADC_START_SEL);
	#endif
	/*mt6325_upmu_get_rg_adc_out_wakeup_swchr(); */
	adc_result = (adc_result_reg * r_val_temp * VOLTAGE_FULL_RANGE) / ADC_PRECISE;
	bm_err("[oam] get_hw_ocv (swchr) : adc_result_reg=%d, adc_result=%d, start_sel=%d\n",
	adc_result_reg, adc_result, reg_sel);
#else
	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		adc_result_reg = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_WAKEUP_PCHR);
		reg_sel = pmic_get_register_value(PMIC_RG_STRUP_AUXADC_START_SEL);
	#else
		adc_result_reg = pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_OUT_WAKEUP_PCHR);
		reg_sel = pmic_get_register_value(MT6351_PMIC_STRUP_AUXADC_START_SEL);
	#endif
	/*mt6325_upmu_get_rg_adc_out_wakeup_pchr(); */
	adc_result = (adc_result_reg * r_val_temp * VOLTAGE_FULL_RANGE) / ADC_PRECISE;
	bm_err("[oam] get_hw_ocv (pchr) : adc_result_reg=%d, adc_result=%d, start_sel=%d\n",
	adc_result_reg, adc_result, reg_sel);
#endif
	adc_result += g_hw_ocv_tune_value;
	return adc_result;
#endif
}


/*============================================================//*/

#if defined(CONFIG_POWER_EXT)
 /**/
#else

static void dump_nter(void)
{
	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	  bm_print(BM_LOG_CRTI, "[dump_nter] upmu_get_fg_nter_29_16 = 0x%x\r\n",
		   pmic_get_register_value(PMIC_FG_NTER_32_17));
	  bm_print(BM_LOG_CRTI, "[dump_nter] upmu_get_fg_nter_15_00 = 0x%x\r\n",
		   pmic_get_register_value(PMIC_FG_CAR_18_03));
	#else
	  bm_print(BM_LOG_CRTI, "[dump_nter] MT6351_upmu_get_fg_nter_29_16 = 0x%x\r\n",
		 pmic_get_register_value(MT6351_PMIC_FG_NTER_32_17));
	bm_print(BM_LOG_CRTI, "[dump_nter] MT6351_upmu_get_fg_nter_15_00 = 0x%x\r\n",
		 pmic_get_register_value(MT6351_PMIC_FG_CAR_18_03));
	#endif
}

static void dump_car(void)
{
	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	  bm_print(BM_LOG_CRTI, "[dump_car] upmu_get_fg_car_31_16 = 0x%x\r\n",
		   pmic_get_register_value(PMIC_FG_CAR_34_19));
	  bm_print(BM_LOG_CRTI, "[dump_car] upmu_get_fg_car_15_00 = 0x%x\r\n",
		   pmic_get_register_value(PMIC_FG_CAR_18_03));
	#else
	bm_print(BM_LOG_CRTI, "[dump_car] upmu_get_fg_car_31_16 = 0x%x\r\n",
		 pmic_get_register_value(MT6351_PMIC_FG_CAR_34_19));
	bm_print(BM_LOG_CRTI, "[dump_car] upmu_get_fg_car_15_00 = 0x%x\r\n",
		 pmic_get_register_value(MT6351_PMIC_FG_CAR_18_03));
	#endif
}

static unsigned int fg_get_data_ready_status(void)
{
	unsigned int ret = 0;
	unsigned int temp_val = 0;
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	ret = pmic_read_interface(PMIC_FG_LATCHDATA_ST_ADDR, &temp_val, 0xFFFF, 0x0);

	bm_print(BM_LOG_FULL, "[fg_get_data_ready_status] Reg[0x%x]=0x%x\r\n", PMIC_FG_LATCHDATA_ST_ADDR,
		 temp_val);

	temp_val =
	(temp_val & (PMIC_FG_LATCHDATA_ST_MASK << PMIC_FG_LATCHDATA_ST_SHIFT))
	>> PMIC_FG_LATCHDATA_ST_SHIFT;
#else
	ret = pmic_read_interface(MT6351_PMIC_FG_LATCHDATA_ST_ADDR, &temp_val, 0xFFFF, 0x0);

	bm_print(BM_LOG_FULL, "[fg_get_data_ready_status] Reg[0x%x]=0x%x\r\n", MT6351_PMIC_FG_LATCHDATA_ST_ADDR,
		 temp_val);

	temp_val =
	(temp_val & (MT6351_PMIC_FG_LATCHDATA_ST_MASK << MT6351_PMIC_FG_LATCHDATA_ST_SHIFT))
	>> MT6351_PMIC_FG_LATCHDATA_ST_SHIFT;
#endif

	return temp_val;
}
#endif

static signed int fgauge_read_current(void *data);
static signed int fgauge_initialization(void *data)
{
#if defined(CONFIG_POWER_EXT)
	/* */
#else
	unsigned int ret = 0;
	signed int current_temp = 0;
	int m = 0;

	get_hw_chip_diff_trim_value();

	/* 1. HW initialization
	 * FGADC clock is 32768Hz from RTC
	 * Enable FGADC in current mode at 32768Hz with auto-calibration
	 * (1)    Enable VA2
	 * (2)    Enable FGADC clock for digital
	 */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	  pmic_set_register_value(PMIC_CLK_FGADC_ANA_CK_PDN, 0);
	  /*   mt6325_upmu_set_rg_fgadc_ana_ck_pdn(0);*/
	  pmic_set_register_value(PMIC_CLK_FGADC_DIG_CK_PDN, 0);
	  /*    mt6325_upmu_set_rg_fgadc_dig_ck_pdn(0);*/

#else
	pmic_set_register_value(MT6351_PMIC_RG_FGADC_ANA_CK_PDN, 0);
	/*   mt6325_upmu_set_rg_fgadc_ana_ck_pdn(0);*/
	pmic_set_register_value(MT6351_PMIC_RG_FGADC_DIG_CK_PDN, 0);
	/*    mt6325_upmu_set_rg_fgadc_dig_ck_pdn(0);*/
#endif
	/*(3)    Set current mode, auto-calibration mode and 32KHz clock source */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	ret = pmic_config_interface(MT6353_FGADC_CON0, 0x0028, 0x00FF, 0x0);
#else
	ret = pmic_config_interface(MT6351_FGADC_CON0, 0x0028, 0x00FF, 0x0);
#endif
	/*(4)    Enable FGADC */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	ret = pmic_config_interface(MT6353_FGADC_CON0, 0x0029, 0x00FF, 0x0);
#else
	ret = pmic_config_interface(MT6351_FGADC_CON0, 0x0029, 0x00FF, 0x0);
#endif

	/*reset HW FG */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	ret = pmic_config_interface(MT6353_FGADC_CON0, 0x7100, 0xFF00, 0x0);
#else
	ret = pmic_config_interface(MT6351_FGADC_CON0, 0x7100, 0xFF00, 0x0);
#endif
	bm_print(BM_LOG_CRTI, "******** [fgauge_initialization] reset HW FG!\n");

	/*set FG_OSR */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	pmic_set_register_value(PMIC_FG_OSR, 0x8);
	bm_print(BM_LOG_CRTI, "[fgauge_initialization] Reg[0x%x]=0x%x\n", PMIC_FG_OSR_ADDR,
		 upmu_get_reg_value(PMIC_FG_OSR_ADDR));
#else
	pmic_set_register_value(MT6351_PMIC_FG_OSR, 0x8);
	bm_print(BM_LOG_CRTI, "[fgauge_initialization] Reg[0x%x]=0x%x\n", MT6351_PMIC_FG_OSR_ADDR,
		 upmu_get_reg_value(MT6351_PMIC_FG_OSR_ADDR));
#endif

	/*make sure init finish */
	m = 0;
		while (current_temp == 0) {
			fgauge_read_current(&current_temp);
			m++;
			if (m > 1000) {
				bm_err("[fgauge_initialization] timeout!\r\n");
				break;
			}
		}

	bm_print(BM_LOG_CRTI, "******** [fgauge_initialization] Done!\n");
#endif

	return STATUS_OK;
}

static signed int fgauge_read_current(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 0;
#else
	unsigned short uvalue16 = 0;
	signed int dvalue = 0;
	int m = 0;
	long long Temp_Value = 0;
	signed int Current_Compensate_Value = 0;
	unsigned int ret = 0;


	fgadc_hal_lock();
	/* HW Init
	 * (1)    i2c_write (0x60, 0xC8, 0x01); // Enable VA2
	 * (2)    i2c_write (0x61, 0x15, 0x00); // Enable FGADC clock for digital
	 * (3)    i2c_write (0x61, 0x69, 0x28); // Set current mode, auto-calibration mode and 32KHz clock source
	 * (4)    i2c_write (0x61, 0x69, 0x29); // Enable FGADC

	 * Read HW Raw Data
	 * (1)    Set READ command
	 */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	 ret = pmic_config_interface(MT6353_FGADC_CON0, 0x0200, 0xFF00, 0x0);
#else
	 ret = pmic_config_interface(MT6351_FGADC_CON0, 0x0200, 0xFF00, 0x0);
#endif
	/* (2)     Keep i2c read when status = 1 (0x06) */
	m = 0;
		while (fg_get_data_ready_status() == 0) {
			m++;
			if (m > 1000) {
				bm_err(
				"[fgauge_read_current] fg_get_data_ready_status timeout 1 !\r\n");
				break;
			}
		}
	/*
	 * (3)    Read FG_CURRENT_OUT[15:08]
	 * (4)    Read FG_CURRENT_OUT[07:00]
	 */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	  uvalue16 = pmic_get_register_value(PMIC_FG_CURRENT_OUT);	/*mt6325_upmu_get_fg_current_out(); */

#else
	uvalue16 = pmic_get_register_value(MT6351_PMIC_FG_CURRENT_OUT);	/*mt6325_upmu_get_fg_current_out(); */
#endif
	bm_print(BM_LOG_FULL, "[fgauge_read_current] : FG_CURRENT = %x\r\n", uvalue16);
	/*
	 * (5)    (Read other data)
	 * (6)    Clear status to 0
	 */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	ret = pmic_config_interface(MT6353_FGADC_CON0, 0x0800, 0xFF00, 0x0);
#else
	ret = pmic_config_interface(MT6351_FGADC_CON0, 0x0800, 0xFF00, 0x0);
#endif
	/*
	 * (7)    Keep i2c read when status = 0 (0x08)
	 * while ( fg_get_sw_clear_status() != 0 )
	 */
	m = 0;
		while (fg_get_data_ready_status() != 0) {
			m++;
			if (m > 1000) {
				bm_err(
				"[fgauge_read_current] fg_get_data_ready_status timeout 2 !\r\n");
				break;
			}
		}
	/*(8)    Recover original settings */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	ret = pmic_config_interface(MT6353_FGADC_CON0, 0x0000, 0xFF00, 0x0);
#else
	ret = pmic_config_interface(MT6351_FGADC_CON0, 0x0000, 0xFF00, 0x0);
#endif
	fgadc_hal_unlock();


	/*calculate the real world data    */
	dvalue = (unsigned int) uvalue16;
		if (dvalue == 0) {
			Temp_Value = (long long) dvalue;
			g_fg_is_charging = KAL_FALSE;
		} else if (dvalue > 32767) {
			/* > 0x8000 */
			Temp_Value = (long long) (dvalue - 65535);
			Temp_Value = Temp_Value - (Temp_Value * 2);
			g_fg_is_charging = KAL_FALSE;
		} else {
			Temp_Value = (long long) dvalue;
			g_fg_is_charging = KAL_TRUE;
		}

	Temp_Value = Temp_Value * UNIT_FGCURRENT;
	do_div(Temp_Value, 100000);
	dvalue = (unsigned int) Temp_Value;

	if (g_fg_is_charging == KAL_TRUE)
		bm_print(BM_LOG_FULL, "[fgauge_read_current] current(charging) = %d mA\r\n",
			 dvalue);
	else
		bm_print(BM_LOG_FULL, "[fgauge_read_current] current(discharging) = %d mA\r\n",
			 dvalue);

		/* Auto adjust value */
		if (batt_meter_cust_data.r_fg_value != 20) {
			bm_print(BM_LOG_FULL,
			"[fgauge_read_current] Auto adjust value due to the Rfg is %d\n Ori current=%d, ",
			batt_meter_cust_data.r_fg_value, dvalue);

			dvalue = (dvalue * 20) / batt_meter_cust_data.r_fg_value;

			bm_print(BM_LOG_FULL, "[fgauge_read_current] new current=%d\n", dvalue);
		}

		/* K current  */
		if (batt_meter_cust_data.r_fg_board_slope != batt_meter_cust_data.r_fg_board_base)
			dvalue =
			((dvalue * batt_meter_cust_data.r_fg_board_base) +
			(batt_meter_cust_data.r_fg_board_slope / 2)) /
			batt_meter_cust_data.r_fg_board_slope;

		/* current compensate */
		if (g_fg_is_charging == KAL_TRUE)
			dvalue = dvalue + Current_Compensate_Value;
		else
			dvalue = dvalue - Current_Compensate_Value;


		bm_print(BM_LOG_FULL, "[fgauge_read_current] ori current=%d\n", dvalue);

		dvalue = ((dvalue * batt_meter_cust_data.car_tune_value) / 100);

		dvalue = use_chip_trim_value(dvalue);

		bm_print(BM_LOG_FULL, "[fgauge_read_current] final current=%d (ratio=%d)\n",
			 dvalue, batt_meter_cust_data.car_tune_value);

		*(signed int *) (data) = dvalue;
#endif

		return STATUS_OK;
	}



signed int fgauge_read_IM_current(void *data)
{
#if defined(CONFIG_POWER_EXT)
		*(signed int *) (data) = 0;
#else
		unsigned short uvalue16 = 0;
		signed int dvalue = 0;
		/*int m = 0;*/
		long long Temp_Value = 0;
		signed int Current_Compensate_Value = 0;
		/*unsigned int ret = 0;*/
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		uvalue16 = pmic_get_register_value(PMIC_FG_R_CURR);
#else
		uvalue16 = pmic_get_register_value(MT6351_PMIC_FG_R_CURR);
#endif

		bm_print(BM_LOG_FULL, "[fgauge_read_IM_current] : FG_CURRENT = %x\r\n",
			 uvalue16);

		/*calculate the real world data    */
		dvalue = (unsigned int) uvalue16;
		if (dvalue == 0) {
			Temp_Value = (long long) dvalue;
			g_fg_is_charging = KAL_FALSE;
		} else if (dvalue > 32767) {
			/* > 0x8000 */
			Temp_Value = (long long) (dvalue - 65535);
			Temp_Value = Temp_Value - (Temp_Value * 2);
			g_fg_is_charging = KAL_FALSE;
		} else {
			Temp_Value = (long long) dvalue;
			g_fg_is_charging = KAL_TRUE;
		}

		Temp_Value = Temp_Value * UNIT_FGCURRENT;
		do_div(Temp_Value, 100000);
		dvalue = (unsigned int) Temp_Value;

		if (g_fg_is_charging == KAL_TRUE)
			bm_print(BM_LOG_FULL,
			"[fgauge_read_IM_current] current(charging) = %d mA\r\n",
			dvalue);
		else
			bm_print(BM_LOG_FULL,
			"[fgauge_read_IM_current] current(discharging) = %d mA\r\n",
			dvalue);

		/* Auto adjust value */
		if (batt_meter_cust_data.r_fg_value != 20) {
			bm_print(BM_LOG_FULL,
			"[fgauge_read_IM_current] Auto adjust value due to the Rfg is %d\n Ori current=%d, ",
			batt_meter_cust_data.r_fg_value, dvalue);

			dvalue = (dvalue * 20) / batt_meter_cust_data.r_fg_value;

			bm_print(BM_LOG_FULL, "[fgauge_read_IM_current] new current=%d\n",
			dvalue);
		}

		/* K current */
		if (batt_meter_cust_data.r_fg_board_slope != batt_meter_cust_data.r_fg_board_base)
			dvalue =
			((dvalue * batt_meter_cust_data.r_fg_board_base) +
			(batt_meter_cust_data.r_fg_board_slope / 2)) /
			batt_meter_cust_data.r_fg_board_slope;

		/* current compensate */
		if (g_fg_is_charging == KAL_TRUE)
			dvalue = dvalue + Current_Compensate_Value;
		else
			dvalue = dvalue - Current_Compensate_Value;

		bm_print(BM_LOG_FULL, "[fgauge_read_IM_current] ori current=%d\n", dvalue);

		dvalue = ((dvalue * batt_meter_cust_data.car_tune_value) / 100);

		dvalue = use_chip_trim_value(dvalue);

		bm_print(BM_LOG_FULL, "[fgauge_read_IM_current] final current=%d (ratio=%d)\n",
			 dvalue, batt_meter_cust_data.car_tune_value);

		*(signed int *) (data) = dvalue;
#endif

		return STATUS_OK;
}


static signed int fgauge_read_current_sign(void *data)
{
	*(kal_bool *) (data) = g_fg_is_charging;

	return STATUS_OK;
}

static signed int fgauge_read_columb(void *data);

signed int fgauge_set_columb_interrupt_internal(void *data, int reset)
{
#if defined(CONFIG_POWER_EXT)
		return STATUS_OK;
#else

	unsigned int uvalue32_CAR = 0;
	unsigned int uvalue32_CAR_MSB = 0;
	signed int upperbound = 0, lowbound = 0;

	signed short m;
	unsigned int car = *(unsigned int *) (data);
	unsigned int ret = 0;
	signed int value32_CAR;

	bm_print(BM_LOG_FULL, "fgauge_set_columb_interrupt_internal car=%d\n", car);


	if (car == 0) {
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		pmic_set_register_value(PMIC_RG_INT_EN_FG_BAT_H, 0);
		pmic_set_register_value(PMIC_RG_INT_EN_FG_BAT_L, 0);
		bm_print(BM_LOG_CRTI,
			"[fgauge_set_columb_interrupt] low:[0xcae]=0x%x 0x%x  high:[0xcb0]=0x%x 0x%x now:0x%x 0x%x %d %d \r\n",
			pmic_get_register_value(PMIC_FG_BLTR_15_00),
			pmic_get_register_value(PMIC_FG_BLTR_31_16),
			pmic_get_register_value(PMIC_FG_BFTR_15_00),
			pmic_get_register_value(PMIC_FG_BFTR_31_16),
			pmic_get_register_value(PMIC_FG_CAR_18_03),
			pmic_get_register_value(PMIC_FG_CAR_34_19),
			pmic_get_register_value(PMIC_RG_INT_EN_FG_BAT_L),
			pmic_get_register_value(PMIC_RG_INT_EN_FG_BAT_H));

#else
		pmic_set_register_value(MT6351_PMIC_RG_INT_EN_FG_BAT_H, 0);
		pmic_set_register_value(MT6351_PMIC_RG_INT_EN_FG_BAT_L, 0);
		bm_print(BM_LOG_CRTI,
			"[fgauge_set_columb_interrupt] low:[0xcae]=0x%x 0x%x  high:[0xcb0]=0x%x 0x%x now:0x%x 0x%x %d %d \r\n",
			pmic_get_register_value(MT6351_PMIC_FG_BLTR_15_00),
			pmic_get_register_value(MT6351_PMIC_FG_BLTR_31_16),
			pmic_get_register_value(MT6351_PMIC_FG_BFTR_15_00),
			pmic_get_register_value(MT6351_PMIC_FG_BFTR_31_16),
			pmic_get_register_value(MT6351_PMIC_FG_CAR_18_03),
			pmic_get_register_value(MT6351_PMIC_FG_CAR_34_19),
			pmic_get_register_value(MT6351_PMIC_RG_INT_EN_FG_BAT_L),
			pmic_get_register_value(MT6351_PMIC_RG_INT_EN_FG_BAT_H));
#endif
		return STATUS_OK;
	}

	if (car == 0x1ffff) {
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		pmic_set_register_value(PMIC_RG_INT_EN_FG_BAT_H, 1);
		pmic_set_register_value(PMIC_RG_INT_EN_FG_BAT_L, 1);
		bm_print(BM_LOG_CRTI,
			"[fgauge_set_columb_interrupt] low:[0xcae]=0x%x 0x%x  high:[0xcb0]=0x%x 0x%x now:0x%x 0x%x %d %d \r\n",
			pmic_get_register_value(PMIC_FG_BLTR_15_00),
			pmic_get_register_value(PMIC_FG_BLTR_31_16),
			pmic_get_register_value(PMIC_FG_BFTR_15_00),
			pmic_get_register_value(PMIC_FG_BFTR_31_16),
			pmic_get_register_value(PMIC_FG_CAR_18_03),
			pmic_get_register_value(PMIC_FG_CAR_34_19),
			pmic_get_register_value(PMIC_RG_INT_EN_FG_BAT_L),
			pmic_get_register_value(PMIC_RG_INT_EN_FG_BAT_H));

#else
		pmic_set_register_value(MT6351_PMIC_RG_INT_EN_FG_BAT_H, 1);
		pmic_set_register_value(MT6351_PMIC_RG_INT_EN_FG_BAT_L, 1);
			bm_print(BM_LOG_CRTI,
			"[fgauge_set_columb_interrupt] low:[0xcae]=0x%x 0x%x  high:[0xcb0]=0x%x 0x%x now:0x%x 0x%x %d %d \r\n",
			pmic_get_register_value(MT6351_PMIC_FG_BLTR_15_00),
			pmic_get_register_value(MT6351_PMIC_FG_BLTR_31_16),
			pmic_get_register_value(MT6351_PMIC_FG_BFTR_15_00),
			pmic_get_register_value(MT6351_PMIC_FG_BFTR_31_16),
			pmic_get_register_value(MT6351_PMIC_FG_CAR_18_03),
			pmic_get_register_value(MT6351_PMIC_FG_CAR_34_19),
			pmic_get_register_value(MT6351_PMIC_RG_INT_EN_FG_BAT_L),
			pmic_get_register_value(MT6351_PMIC_RG_INT_EN_FG_BAT_H));
#endif
		return STATUS_OK;
	}

/*
 * HW Init
 * (1)    i2c_write (0x60, 0xC8, 0x01); // Enable VA2
 * (2)    i2c_write (0x61, 0x15, 0x00); // Enable FGADC clock for digital
 * (3)    i2c_write (0x61, 0x69, 0x28); // Set current mode, auto-calibration mode and 32KHz clock source
 * (4)    i2c_write (0x61, 0x69, 0x29); // Enable FGADC

 * Read HW Raw Data
 * (1)    Set READ command
 */

	fgadc_hal_lock();

#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	if (reset == 0)
		ret = pmic_config_interface(MT6353_FGADC_CON0, 0x0200, 0xFF00, 0x0);
	else
		ret = pmic_config_interface(MT6353_FGADC_CON0, 0x7300, 0xFF00, 0x0);
#else
	if (reset == 0)
		ret = pmic_config_interface(MT6351_FGADC_CON0, 0x0200, 0xFF00, 0x0);
	else
		ret = pmic_config_interface(MT6351_FGADC_CON0, 0x7300, 0xFF00, 0x0);
#endif
	/*(2)    Keep i2c read when status = 1 (0x06) */
	m = 0;
	while (fg_get_data_ready_status() == 0) {
		m++;
		if (m > 1000) {
			bm_err(
				 "[fgauge_set_columb_interrupt] fg_get_data_ready_status timeout 1 !");
			break;
		}
	}
/*
 * (3)    Read FG_CURRENT_OUT[28:14]
 * (4)    Read FG_CURRENT_OUT[31]
 */
	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	value32_CAR = (pmic_get_register_value(PMIC_FG_CAR_18_03));
	value32_CAR |= ((pmic_get_register_value(PMIC_FG_CAR_34_19)) & 0xffff) << 16;

	uvalue32_CAR_MSB = (pmic_get_register_value(PMIC_FG_CAR_34_19) & 0x8000) >> 15;

	bm_print(BM_LOG_CRTI,
		 "[fgauge_set_columb_interrupt] FG_CAR = 0x%x   uvalue32_CAR_MSB:0x%x 0x%x 0x%x\r\n",
		 uvalue32_CAR, uvalue32_CAR_MSB, (pmic_get_register_value(PMIC_FG_CAR_18_03)),
		 (pmic_get_register_value(PMIC_FG_CAR_34_19)));

	#else
	value32_CAR = (pmic_get_register_value(MT6351_PMIC_FG_CAR_18_03));
	value32_CAR |= ((pmic_get_register_value(MT6351_PMIC_FG_CAR_34_19)) & 0xffff) << 16;

	uvalue32_CAR_MSB = (pmic_get_register_value(MT6351_PMIC_FG_CAR_34_19) & 0x8000) >> 15;

	bm_print(BM_LOG_CRTI,
		"[fgauge_set_columb_interrupt] FG_CAR = 0x%x   uvalue32_CAR_MSB:0x%x 0x%x 0x%x\r\n",
		uvalue32_CAR, uvalue32_CAR_MSB, (pmic_get_register_value(MT6351_PMIC_FG_CAR_18_03)),
		(pmic_get_register_value(MT6351_PMIC_FG_CAR_34_19)));
	#endif

	/*
	 * (5)	  (Read other data)
	 * (6)	  Clear status to 0
	 */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		ret = pmic_config_interface(MT6353_FGADC_CON0, 0x0800, 0xFF00, 0x0);
#else
		ret = pmic_config_interface(MT6351_FGADC_CON0, 0x0800, 0xFF00, 0x0);
#endif
	/*
	 * (7)	  Keep i2c read when status = 0 (0x08)
	 * while ( fg_get_sw_clear_status() != 0 )
	 */
		m = 0;
		while (fg_get_data_ready_status() != 0) {
			m++;
			if (m > 1000) {
				bm_err(
					 "[fgauge_set_columb_interrupt] fg_get_data_ready_status timeout 2 !\r\n");
				break;
			}
		}
		/*(8)	 Recover original settings */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		ret = pmic_config_interface(MT6353_FGADC_CON0, 0x0000, 0xFF00, 0x0);
#else
		ret = pmic_config_interface(MT6351_FGADC_CON0, 0x0000, 0xFF00, 0x0);
#endif
	fgadc_hal_unlock();

	/*restore use_chip_trim_value() */
	car = car * 0x4d14;

	upperbound = value32_CAR;
	lowbound = value32_CAR;


	bm_print(BM_LOG_CRTI,
		 "[fgauge_set_columb_interrupt] upper = 0x%x:%d  low=0x%x:%d  car=0x%x:%d\r\n",
		 upperbound, upperbound, lowbound, lowbound, car, car);

	upperbound = upperbound + car;
	lowbound = lowbound - car;

	bm_print(BM_LOG_CRTI,
		 "[fgauge_set_columb_interrupt]final upper = 0x%x:%d  low=0x%x:%d  car=0x%x:%d\r\n",
		 upperbound, upperbound, lowbound, lowbound, car, car);
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	pmic_set_register_value(PMIC_FG_BLTR_15_00, lowbound & 0xffff);
	pmic_set_register_value(PMIC_FG_BLTR_31_16, (lowbound & 0xffff0000) >> 16);
	pmic_set_register_value(PMIC_FG_BFTR_15_00, upperbound & 0xffff);
	pmic_set_register_value(PMIC_FG_BFTR_31_16, (upperbound & 0xffff0000) >> 16);
	mdelay(1);
	pmic_set_register_value(PMIC_RG_INT_EN_FG_BAT_H, 1);
	pmic_set_register_value(PMIC_RG_INT_EN_FG_BAT_L, 1);


	bm_print(BM_LOG_CRTI,
		 "[fgauge_set_columb_interrupt] low:[0xcae]=0x%x 0x%x   high:[0xcb0]=0x%x 0x%x\r\n",
		 pmic_get_register_value(PMIC_FG_BLTR_15_00), pmic_get_register_value(PMIC_FG_BLTR_31_16),
		 pmic_get_register_value(PMIC_FG_BFTR_15_00), pmic_get_register_value(PMIC_FG_BFTR_31_16));

#else
	pmic_set_register_value(MT6351_PMIC_FG_BLTR_15_00, lowbound & 0xffff);
	pmic_set_register_value(MT6351_PMIC_FG_BLTR_31_16, (lowbound & 0xffff0000) >> 16);
	pmic_set_register_value(MT6351_PMIC_FG_BFTR_15_00, upperbound & 0xffff);
	pmic_set_register_value(MT6351_PMIC_FG_BFTR_31_16, (upperbound & 0xffff0000) >> 16);
	mdelay(1);
	pmic_set_register_value(MT6351_PMIC_RG_INT_EN_FG_BAT_H, 1);
	pmic_set_register_value(MT6351_PMIC_RG_INT_EN_FG_BAT_L, 1);


	bm_print(BM_LOG_CRTI,
		 "[fgauge_set_columb_interrupt] low:[0xcae]=0x%x 0x%x   high:[0xcb0]=0x%x 0x%x\r\n",
		pmic_get_register_value(MT6351_PMIC_FG_BLTR_15_00),
		pmic_get_register_value(MT6351_PMIC_FG_BLTR_31_16),
		pmic_get_register_value(MT6351_PMIC_FG_BFTR_15_00),
		pmic_get_register_value(MT6351_PMIC_FG_BFTR_31_16));
#endif

	return STATUS_OK;
#endif

}


static signed int fgauge_set_columb_interrupt(void *data)
{
	return fgauge_set_columb_interrupt_internal(data, 0);
}

static signed int fgauge_read_columb_internal(void *data, int reset, int precise)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 0;
#else
	unsigned int uvalue32_CAR = 0;
	unsigned int uvalue32_CAR_MSB = 0;
	signed int dvalue_CAR = 0;
	int m = 0;
	long long Temp_Value = 0;
	unsigned int ret = 0;

/*
 * HW Init
 * (1)    i2c_write (0x60, 0xC8, 0x01); // Enable VA2
 * (2)    i2c_write (0x61, 0x15, 0x00); // Enable FGADC clock for digital
 * (3)    i2c_write (0x61, 0x69, 0x28); // Set current mode, auto-calibration mode and 32KHz clock source
 * (4)    i2c_write (0x61, 0x69, 0x29); // Enable FGADC

 * Read HW Raw Data
 * (1)    Set READ command
 */
	fgadc_hal_lock();
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	if (reset == 0)
		ret = pmic_config_interface(MT6353_FGADC_CON0, 0x0200, 0xFF00, 0x0);
	else
		ret = pmic_config_interface(MT6353_FGADC_CON0, 0x7300, 0xFF00, 0x0);
#else
	if (reset == 0)
		ret = pmic_config_interface(MT6351_FGADC_CON0, 0x0200, 0xFF00, 0x0);
	else
		ret = pmic_config_interface(MT6351_FGADC_CON0, 0x7300, 0xFF00, 0x0);
#endif

	/*(2)    Keep i2c read when status = 1 (0x06) */
	m = 0;
	while (fg_get_data_ready_status() == 0) {
		m++;
		if (m > 1000) {
			bm_err(
				 "[fgauge_read_columb_internal] fg_get_data_ready_status timeout 1 !\r\n");
			break;
		}
	}
/*
 * (3)    Read FG_CURRENT_OUT[28:14]
 * (4)    Read FG_CURRENT_OUT[31]
 */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	uvalue32_CAR =  (pmic_get_register_value(PMIC_FG_CAR_18_03)) >> 11;
	uvalue32_CAR |= ((pmic_get_register_value(PMIC_FG_CAR_34_19)) & 0x0FFF) << 5;

	uvalue32_CAR_MSB = (pmic_get_register_value(PMIC_FG_CAR_34_19) & 0x8000) >> 15;
#else
	uvalue32_CAR = (pmic_get_register_value(MT6351_PMIC_FG_CAR_18_03)) >> 11;
	uvalue32_CAR |= ((pmic_get_register_value(MT6351_PMIC_FG_CAR_34_19)) & 0x0FFF) << 5;

	uvalue32_CAR_MSB = (pmic_get_register_value(MT6351_PMIC_FG_CAR_34_19) & 0x8000) >> 15;
#endif

	bm_print(BM_LOG_FULL, "[fgauge_read_columb_internal] FG_CAR = 0x%x\r\n",
		 uvalue32_CAR);
	bm_print(BM_LOG_FULL,
		 "[fgauge_read_columb_internal] uvalue32_CAR_MSB = 0x%x\r\n",
		 uvalue32_CAR_MSB);
/*
 * (5)    (Read other data)
 * (6)    Clear status to 0
 */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	ret = pmic_config_interface(MT6353_FGADC_CON0, 0x0800, 0xFF00, 0x0);
#else
	ret = pmic_config_interface(MT6351_FGADC_CON0, 0x0800, 0xFF00, 0x0);
#endif
/*
 * (7)    Keep i2c read when status = 0 (0x08)
 * while ( fg_get_sw_clear_status() != 0 )
 */
	m = 0;
	while (fg_get_data_ready_status() != 0) {
		m++;
		if (m > 1000) {
			bm_err(
				 "[fgauge_read_columb_internal] fg_get_data_ready_status timeout 2 !\r\n");
			break;
		}
	}
	/*(8)    Recover original settings */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	ret = pmic_config_interface(MT6353_FGADC_CON0, 0x0000, 0xFF00, 0x0);
#else
	ret = pmic_config_interface(MT6351_FGADC_CON0, 0x0000, 0xFF00, 0x0);
#endif
	fgadc_hal_unlock();

/*calculate the real world data    */
	dvalue_CAR = (signed int) uvalue32_CAR;

	if (uvalue32_CAR == 0) {
		Temp_Value = 0;
	} else if (uvalue32_CAR == 0x1ffff) {
		Temp_Value = 0;
	} else if (uvalue32_CAR_MSB == 0x1) {
		/* dis-charging */
		Temp_Value = (long long) (dvalue_CAR - 0x1ffff);	/* keep negative value */
		Temp_Value = Temp_Value - (Temp_Value * 2);
	} else {
		/*charging */
		Temp_Value = (long long) dvalue_CAR;
	}

#if 0
	Temp_Value = (((Temp_Value * 35986) / 10) + (5)) / 10;	/*[28:14]'s LSB=359.86 uAh */
#else
	Temp_Value = Temp_Value * 35986;
	do_div(Temp_Value, 10);
	Temp_Value = Temp_Value + 5;
	do_div(Temp_Value, 10);
#endif

#if 0
	dvalue_CAR = Temp_Value / 1000;	/*mAh */
#else
	/*dvalue_CAR = (Temp_Value/8)/1000;     mAh, due to FG_OSR=0x8*/
	if (precise == 0) {
		do_div(Temp_Value, 8);
		do_div(Temp_Value, 1000);
	} else {
		do_div(Temp_Value, 8);
		do_div(Temp_Value, 100);
	}

	if (uvalue32_CAR_MSB == 0x1)
		dvalue_CAR = (signed int) (Temp_Value - (Temp_Value * 2));	/* keep negative value */
	else
		dvalue_CAR = (signed int) Temp_Value;
#endif

	bm_print(BM_LOG_FULL, "[fgauge_read_columb_internal] dvalue_CAR = %d\r\n",
		 dvalue_CAR);

	/*#if (OSR_SELECT_7 == 1) */



/*Auto adjust value*/
	if (batt_meter_cust_data.r_fg_value != 20) {
		bm_print(BM_LOG_FULL,
			 "[fgauge_read_columb_internal] Auto adjust value deu to the Rfg is %d\n Ori CAR=%d, ",
			 batt_meter_cust_data.r_fg_value, dvalue_CAR);

		dvalue_CAR = (dvalue_CAR * 20) / batt_meter_cust_data.r_fg_value;

		bm_print(BM_LOG_FULL, "[fgauge_read_columb_internal] new CAR=%d\n",
			 dvalue_CAR);
	}

	dvalue_CAR = ((dvalue_CAR * batt_meter_cust_data.car_tune_value) / 100);

	dvalue_CAR = use_chip_trim_value(dvalue_CAR);

	bm_print(BM_LOG_FULL, "[fgauge_read_columb_internal] final dvalue_CAR = %d\r\n",
		 dvalue_CAR);

	dump_nter();
	dump_car();

	*(signed int *) (data) = dvalue_CAR;
#endif

	return STATUS_OK;
}

static signed int fgauge_read_columb(void *data)
{
	return fgauge_read_columb_internal(data, 0, 0);
}

static signed int fgauge_read_columb_accurate(void *data)
{

#if defined(SOC_BY_3RD_FG)
	*(kal_int32 *)(data) = bq27531_get_remaincap();
	return STATUS_OK;

#else
	return fgauge_read_columb_internal(data, 0, 1);
#endif
}


static signed int fgauge_hw_reset(void *data)
{
#if defined(CONFIG_POWER_EXT)
	 /**/
#else
	volatile unsigned int val_car = 1;
	unsigned int val_car_temp = 1;
	unsigned int ret = 0;

	bm_print(BM_LOG_FULL, "[fgauge_hw_reset] : Start \r\n");

	while (val_car != 0x0) {
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		ret = pmic_config_interface(MT6353_FGADC_CON0, 0x7100, 0xFF00, 0x0);
#else
		ret = pmic_config_interface(MT6351_FGADC_CON0, 0x7100, 0xFF00, 0x0);
#endif
		fgauge_read_columb_internal(&val_car_temp, 1, 0);
		val_car = val_car_temp;
		bm_print(BM_LOG_FULL, "#");
	}

	bm_print(BM_LOG_FULL, "[fgauge_hw_reset] : End \r\n");
#endif

	return STATUS_OK;
}

static signed int read_adc_v_bat_sense(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 4201;
#else
#if defined(SWCHR_POWER_PATH)
	*(signed int *) (data) =
		PMIC_IMM_GetOneChannelValue(PMIC_AUX_ISENSE_AP, *(signed int *) (data), 1);
#else
	*(signed int *) (data) =
		PMIC_IMM_GetOneChannelValue(PMIC_AUX_BATSNS_AP, *(signed int *) (data), 1);
#endif
#endif

	return STATUS_OK;
}



static signed int read_adc_v_i_sense(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 4202;
#else
#if defined(SWCHR_POWER_PATH)
	*(signed int *) (data) =
	PMIC_IMM_GetOneChannelValue(PMIC_AUX_BATSNS_AP, *(signed int *) (data), 1);
#else
	*(signed int *) (data) =
	PMIC_IMM_GetOneChannelValue(PMIC_AUX_ISENSE_AP, *(signed int *) (data), 1);
#endif
#endif

	return STATUS_OK;
}

static signed int read_adc_v_bat_temp(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 0;
#else
#if defined(MTK_PCB_TBAT_FEATURE)
	/* no HW support */
#else
	bm_print(BM_LOG_FULL,
	 "[read_adc_v_bat_temp] return PMIC_IMM_GetOneChannelValue(4,times,1);\n");
	*(signed int *) (data) =
	PMIC_IMM_GetOneChannelValue(PMIC_AUX_BATON_AP, *(signed int *) (data), 1);
#endif
#endif

	return STATUS_OK;
}

static signed int read_adc_v_charger(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 5001;
#else
	signed int val;

	val = PMIC_IMM_GetOneChannelValue(PMIC_AUX_VCDT_AP, *(signed int *) (data), 1);
	val =
		(((batt_meter_cust_data.r_charger_1 +
		batt_meter_cust_data.r_charger_2) * 100 * val) /
		batt_meter_cust_data.r_charger_2) / 100;
	*(signed int *) (data) = val;
#endif

	return STATUS_OK;
}

static signed int read_hw_ocv(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 3999;
#else
#if 0
	*(signed int *) (data) = PMIC_IMM_GetOneChannelValue(AUX_BATSNS_AP, 5, 1);
	bm_print(BM_LOG_CRTI, "[read_hw_ocv] By SW AUXADC for bring up\n");
#else
	*(signed int *) (data) = get_hw_ocv();
#endif
#endif

	return STATUS_OK;
}

static signed int read_is_hw_ocv_ready(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 0;
#else
#if defined(SWCHR_POWER_PATH)
	*(signed int *) (data) = pmic_get_register_value(PMIC_AUXADC_ADC_RDY_WAKEUP_SWCHR);
	bm_err("[read_is_hw_ocv_ready] is_hw_ocv_ready(SWCHR) %d\n", *(signed int *) (data));
	pmic_set_register_value(PMIC_AUXADC_ADC_RDY_WAKEUP_CLR, 1);
	mdelay(1);
	pmic_set_register_value(PMIC_AUXADC_ADC_RDY_WAKEUP_CLR, 0);
#else
	*(signed int *) (data) = pmic_get_register_value(PMIC_AUXADC_ADC_RDY_WAKEUP_PCHR);
	bm_err("[read_is_hw_ocv_ready] is_hw_ocv_ready(PCHR) %d\n", *(signed int *) (data));
	pmic_set_register_value(PMIC_AUXADC_ADC_RDY_WAKEUP_CLR, 1);
	mdelay(1);
	pmic_set_register_value(PMIC_AUXADC_ADC_RDY_WAKEUP_CLR, 0);
#endif
#endif

	return STATUS_OK;
}

static signed int dump_register_fgadc(void *data)
{
	return STATUS_OK;
}

static signed int read_battery_plug_out_status(void *data)
{
	*(signed int *)(data) = is_battery_remove_pmic();

	return STATUS_OK;
}

static signed int fgauge_set_meta_cali_current(void *data)
{
	g_meta_input_cali_current = *(signed int *)(data);

	return STATUS_OK;
}

#if defined(CONFIG_POWER_EXT)
 /**/
#else
static signed int fgauge_get_AUXADC_current_rawdata(unsigned short *uvalue16)
{
	unsigned int ret = 0;
	int m = 0;

	/* (1)    Set READ command */
	fgadc_hal_lock();
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	ret = pmic_config_interface(MT6353_FGADC_CON0, 0x0200, 0xFF00, 0x0);
#else
	ret = pmic_config_interface(MT6351_FGADC_CON0, 0x0200, 0xFF00, 0x0);
#endif

	/*(2)     Keep i2c read when status = 1 (0x06) */
	m = 0;
	while (fg_get_data_ready_status() == 0) {
		m++;
		if (m > 1000) {
			bm_err(
			"[fgauge_get_AUXADC_current_rawdata] fg_get_data_ready_status timeout 1 !\r\n");
			break;
		}
	}

	/* (3)    Read FG_CURRENT_OUT[15:08] */
	/* (4)    Read FG_CURRENT_OUT[07:00] */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	*uvalue16 = pmic_get_register_value(PMIC_FG_CURRENT_OUT);	/*mt6325_upmu_get_fg_current_out(); */

#else
	*uvalue16 = pmic_get_register_value(MT6351_PMIC_FG_CURRENT_OUT);	/*mt6325_upmu_get_fg_current_out(); */
#endif

	/*
	 * (5)    (Read other data)
	 * (6)    Clear status to 0
	 */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	ret = pmic_config_interface(MT6353_FGADC_CON0, 0x0800, 0xFF00, 0x0);
#else
	ret = pmic_config_interface(MT6351_FGADC_CON0, 0x0800, 0xFF00, 0x0);
#endif
	/*
	 * (7)    Keep i2c read when status = 0 (0x08)
	 * while ( fg_get_sw_clear_status() != 0 )
	 */
	m = 0;
		while (fg_get_data_ready_status() != 0) {
			m++;
			if (m > 1000) {
				bm_err(
				 "[fgauge_read_current] fg_get_data_ready_status timeout 2 !\r\n");
				break;
			}
		}
	/*(8)    Recover original settings */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	ret = pmic_config_interface(MT6353_FGADC_CON0, 0x0000, 0xFF00, 0x0);
#else
	ret = pmic_config_interface(MT6351_FGADC_CON0, 0x0000, 0xFF00, 0x0);
#endif
	fgadc_hal_unlock();

	return ret;
}
#endif

static signed int fgauge_meta_cali_car_tune_value(void *data)
{
#if defined(CONFIG_POWER_EXT)
	return STATUS_OK;
#else
	int cali_car_tune;
	long long sum_all = 0;
	long long temp_sum = 0;
	int	avg_cnt = 0;
	int i;
	unsigned short uvalue16;
	unsigned int uvalue32;
	signed int dvalue = 0;
	long long Temp_Value1 = 0;
	long long Temp_Value2 = 0;
	long long current_from_ADC = 0;

	if (g_meta_input_cali_current != 0) {
		for (i = 0; i < 60; i++) {
			if (!fgauge_get_AUXADC_current_rawdata(&uvalue16)) {
				uvalue32 = (unsigned int) uvalue16;
				if (uvalue32 <= 0x8000) {
					Temp_Value1 = (long long)uvalue32;
					pr_err("[111]uvalue16 %d uvalue32 %d Temp_Value1 %lld\n",
						uvalue16, uvalue32, Temp_Value1);
				} else if (uvalue32 > 0x8000) {
					/*Temp_Value1 = (long long) (uvalue32 - 65535);*/
					/*Temp_Value1 = 0 - Temp_Value1;*/
					Temp_Value1 = (long long) (65535 - uvalue32);
					pr_err("[222]uvalue16 %d uvalue32 %d Temp_Value1 %lld\n",
						uvalue16, uvalue32, Temp_Value1);
				}
				sum_all += Temp_Value1;
				avg_cnt++;
				/*****************/
				pr_err("[333]uvalue16 %d uvalue32 %d Temp_Value1 %lld sum_all %lld\n",
						uvalue16, uvalue32, Temp_Value1, sum_all);
				/*****************/
			}
			mdelay(30);
		}
		/*calculate the real world data    */
		/*current_from_ADC = sum_all / avg_cnt;*/
		temp_sum = sum_all;
		pr_err("[444]sum_all %lld temp_sum %lld avg_cnt %d current_from_ADC %lld\n",
			sum_all, temp_sum, avg_cnt, current_from_ADC);

		do_div(temp_sum, avg_cnt);
		current_from_ADC = temp_sum;

		pr_err("[555]sum_all %lld temp_sum %lld avg_cnt %d current_from_ADC %lld\n",
			sum_all, temp_sum, avg_cnt, current_from_ADC);

		Temp_Value2 = current_from_ADC * UNIT_FGCURRENT;

		pr_err("[555]Temp_Value2 %lld current_from_ADC %lld UNIT_FGCURRENT %d\n",
			Temp_Value2, current_from_ADC, UNIT_FGCURRENT);

		do_div(Temp_Value2, 1000000);

		pr_err("[666]Temp_Value2 %lld current_from_ADC %lld UNIT_FGCURRENT %d\n",
			Temp_Value2, current_from_ADC, UNIT_FGCURRENT);

		dvalue = (unsigned int) Temp_Value2;

		/* Auto adjust value */
		if (batt_meter_cust_data.r_fg_value != 20)
			dvalue = (dvalue * 20) / batt_meter_cust_data.r_fg_value;

		pr_err("[666]dvalue %d batt_meter_cust_data.r_fg_value %d\n", dvalue, batt_meter_cust_data.r_fg_value);

		cali_car_tune = g_meta_input_cali_current * 1000 / dvalue;	/* 1000 base, so multiple by 1000*/

		pr_err("[777]dvalue %d batt_meter_cust_data.r_fg_value %d cali_car_tune %d\n",
			dvalue, batt_meter_cust_data.r_fg_value, cali_car_tune);
		*(signed int *) (data) = cali_car_tune;

		pr_err(
			"[fgauge_meta_cali_car_tune_value][%d] meta:%d, adc:%lld, UNI_FGCUR:%d, r_fg_value:%d\n",
			cali_car_tune, g_meta_input_cali_current, current_from_ADC,
			UNIT_FGCURRENT, batt_meter_cust_data.r_fg_value);

		return STATUS_OK;
	}

	return STATUS_UNSUPPORTED;
#endif
}
static signed int(*bm_func[BATTERY_METER_CMD_NUMBER]) (void *data);

signed int bm_ctrl_cmd(BATTERY_METER_CTRL_CMD cmd, void *data)
{
	signed int status = STATUS_UNSUPPORTED;
	static signed int init = -1;

	if (init == -1) {
		init = 0;
		bm_func[BATTERY_METER_CMD_HW_FG_INIT] = fgauge_initialization;
		bm_func[BATTERY_METER_CMD_GET_HW_FG_CURRENT] = fgauge_read_current;
		bm_func[BATTERY_METER_CMD_GET_HW_FG_CURRENT_SIGN] = fgauge_read_current_sign;
		bm_func[BATTERY_METER_CMD_GET_HW_FG_CAR] = fgauge_read_columb;
		bm_func[BATTERY_METER_CMD_HW_RESET] = fgauge_hw_reset;
		bm_func[BATTERY_METER_CMD_GET_ADC_V_BAT_SENSE] = read_adc_v_bat_sense;
		bm_func[BATTERY_METER_CMD_GET_ADC_V_I_SENSE] = read_adc_v_i_sense;
		bm_func[BATTERY_METER_CMD_GET_ADC_V_BAT_TEMP] = read_adc_v_bat_temp;
		bm_func[BATTERY_METER_CMD_GET_ADC_V_CHARGER] = read_adc_v_charger;
		bm_func[BATTERY_METER_CMD_GET_HW_OCV] = read_hw_ocv;
		bm_func[BATTERY_METER_CMD_DUMP_REGISTER] = dump_register_fgadc;
		bm_func[BATTERY_METER_CMD_SET_COLUMB_INTERRUPT] = fgauge_set_columb_interrupt;
		bm_func[BATTERY_METER_CMD_GET_BATTERY_PLUG_STATUS] = read_battery_plug_out_status;
		bm_func[BATTERY_METER_CMD_GET_HW_FG_CAR_ACT] = fgauge_read_columb_accurate;
		bm_func[BATTERY_METER_CMD_GET_IS_HW_OCV_READY] = read_is_hw_ocv_ready;
		bm_func[BATTERY_METER_CMD_SET_META_CALI_CURRENT] = fgauge_set_meta_cali_current;
		bm_func[BATTERY_METER_CMD_META_CALI_CAR_TUNE_VALUE] = fgauge_meta_cali_car_tune_value;
	}

	if (cmd < BATTERY_METER_CMD_NUMBER) {
		if (bm_func[cmd] != NULL)
			status = bm_func[cmd] (data);
		else
			status = STATUS_UNSUPPORTED;
	} else
		status = STATUS_UNSUPPORTED;

	return status;
}
