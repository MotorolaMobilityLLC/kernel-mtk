/*****************************************************************************
 *
 * Filename:
 * ---------
 *    switch_charging.c
 *
 * Project:
 * --------
 *   ALPS_Software
 *
 * Description:
 * ------------
 *   This file implements the interface between BMT and ADC scheduler.
 *
 * Author:
 * -------
 *  Oscar Liu
 *
 *============================================================================
 * Revision:   1.0
 * Modtime:   11 Aug 2005 10:28:16
 * Log:   //mtkvs01/vmdata/Maui_sw/archives/mcu/hal/peripheral/inc/bmt_chr_setting.h-arc
 *
 * 03 05 2015 wy.chuang
 * [ALPS01921641] [L1_merge] for PMIC and charging
 * .
 *             HISTORY
 * Below this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include <linux/types.h>
#include <linux/kernel.h>
#include <mt-plat/battery_meter.h>
#include <mt-plat/battery_common.h>
#include <mt-plat/charging.h>
#include <mach/mt_charging.h>
#include <mt-plat/mt_boot.h>

#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)
#include <linux/mutex.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#if !defined(TA_AC_CHARGING_CURRENT)
#include <mach/mt_pe.h>
#endif
#endif

#ifdef CONFIG_MTK_DUAL_INPUT_CHARGER_SUPPORT
#include <mach/diso.h>
#endif


/* ============================================================ // */
/* define */
/* ============================================================ // */
/* cut off to full */
#define POST_CHARGING_TIME (30*60)	/* 30mins */
#define FULL_CHECK_TIMES 6

/* ============================================================ // */
/* global variable */
/* ============================================================ // */
unsigned int g_bcct_flag = 0;
unsigned int g_bcct_value = 0;
#ifdef CONFIG_MTK_SWITCH_INPUT_OUTPUT_CURRENT_SUPPORT
/*input-output curent distinction*/
unsigned int g_bcct_input_flag = 0;
unsigned int g_bcct_input_value = 0;
#endif
unsigned int g_full_check_count = 0;
CHR_CURRENT_ENUM g_temp_CC_value = CHARGE_CURRENT_0_00_MA;
CHR_CURRENT_ENUM g_temp_input_CC_value = CHARGE_CURRENT_0_00_MA;
unsigned int g_usb_state = USB_UNCONFIGURED;
static bool usb_unlimited;
#if defined(CONFIG_MTK_HAFG_20)
#ifdef HIGH_BATTERY_VOLTAGE_SUPPORT
BATTERY_VOLTAGE_ENUM g_cv_voltage = BATTERY_VOLT_04_340000_V;
#else
BATTERY_VOLTAGE_ENUM g_cv_voltage = BATTERY_VOLT_04_200000_V;
#endif
unsigned int get_cv_voltage(void)
{
	return g_cv_voltage;
}
#endif

 /* ///////////////////////////////////////////////////////////////////////////////////////// */
 /* // PUMP EXPRESS */
 /* ///////////////////////////////////////////////////////////////////////////////////////// */
#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)
struct wake_lock TA_charger_suspend_lock;
kal_bool ta_check_chr_type = KAL_TRUE;
kal_bool ta_cable_out_occur = KAL_FALSE;
kal_bool is_ta_connect = KAL_FALSE;
kal_bool ta_vchr_tuning = KAL_TRUE;
#if defined(PUMPEX_PLUS_RECHG)
kal_bool pep_det_rechg = KAL_FALSE;
#endif
int ta_v_chr_org = 0;
#endif

 /* ///////////////////////////////////////////////////////////////////////////////////////// */
 /* // JEITA */
 /* ///////////////////////////////////////////////////////////////////////////////////////// */
#if defined(CONFIG_MTK_JEITA_STANDARD_SUPPORT)
int g_temp_status = TEMP_POS_10_TO_POS_45;
kal_bool temp_error_recovery_chr_flag = KAL_TRUE;
#endif

/* ============================================================ // */
/* function prototype */
/* ============================================================ // */

/* ============================================================ // */
/* extern variable */
/* ============================================================ // */
/*extern int g_platform_boot_mode; moved to battery_common.h*/

/* ============================================================ // */
/* extern function */
/* ============================================================ // */

/* ============================================================ // */
void BATTERY_SetUSBState(int usb_state_value)
{
#if defined(CONFIG_POWER_EXT)
	battery_log(BAT_LOG_CRTI, "[BATTERY_SetUSBState] in FPGA/EVB, no service\r\n");
#else
	if ((usb_state_value < USB_SUSPEND) || ((usb_state_value > USB_CONFIGURED))) {
		battery_log(BAT_LOG_CRTI,
			    "[BATTERY] BAT_SetUSBState Fail! Restore to default value\r\n");
		usb_state_value = USB_UNCONFIGURED;
	} else {
		battery_log(BAT_LOG_CRTI, "[BATTERY] BAT_SetUSBState Success! Set %d\r\n",
			    usb_state_value);
		g_usb_state = usb_state_value;
	}
#endif
}


unsigned int get_charging_setting_current(void)
{
	return g_temp_CC_value;
}

#if defined(CONFIG_MTK_DYNAMIC_BAT_CV_SUPPORT)
static unsigned int get_constant_voltage(void)
{
	unsigned int cv;
#ifdef CONFIG_MTK_BIF_SUPPORT
	unsigned int vbat_bif;
	unsigned int vbat_auxadc;
	unsigned int vbat, bif_ok;
	int i;
#endif
	/*unit:mV defined in cust_charging.h */
	cv = V_CC2TOPOFF_THRES;
#ifdef CONFIG_MTK_BIF_SUPPORT
	/*Use BIF API to get vbat_core to adjust cv */
	i = 0;
	do {
		battery_charging_control(CHARGING_CMD_GET_BIF_VBAT, &vbat_bif);
		vbat_auxadc = battery_meter_get_battery_voltage(KAL_TRUE);
		if (vbat_bif < vbat_auxadc && vbat_bif != 0) {
			vbat = vbat_bif;
			bif_ok = 1;
			battery_log(BAT_LOG_CRTI, "[BIF]using vbat_bif=%d\n with dV=%dmV", vbat,
				    (vbat_bif - vbat_auxadc));
		} else {
			vbat = vbat_auxadc;
			if (i < 5)
				i++;
			else {
				battery_log(BAT_LOG_CRTI,
					    "[BIF]using vbat_auxadc=%d, check vbat_bif=%d\n", vbat,
					    vbat_bif);
				bif_ok = 0;
				break;
			}
		}
	} while (vbat_bif > vbat_auxadc || vbat_bif == 0);


	/*CV adjustment according to the obtained vbat */
	if (bif_ok == 1) {
#ifdef HIGH_BATTERY_VOLTAGE_SUPPORT
		int vbat1 = 4250;
		int vbat2 = 4300;
		int cv1 = 4450;
#else
		int vbat1 = 4100;
		int vbat2 = 4150;
		int cv1 = 4350;
#endif
		if (vbat >= 3400 && vbat < vbat1)
			cv = 4608;
		else if (vbat >= vbat1 && vbat < vbat2)
			cv = cv1;
		else
			cv = V_CC2TOPOFF_THRES;

		battery_log(BAT_LOG_FULL, "[BIF]dynamic CV=%dmV\n", cv);
	}
#endif
	return cv;
}

static void switch_charger_set_vindpm(unsigned int chr_v)
{
	/*unsigned int delta_v = 0; */
	unsigned int vindpm = 0;

	/*chr_v = battery_meter_get_charger_voltage();*/
	/*delta_v = chr_v - ta_v_chr_org; */

	if (chr_v > 11000)
		vindpm = SWITCH_CHR_VINDPM_12V;
	else if (chr_v > 8000)
		vindpm = SWITCH_CHR_VINDPM_9V;
	else if (chr_v > 6000)
		vindpm = SWITCH_CHR_VINDPM_7V;
	else
		vindpm = SWITCH_CHR_VINDPM_5V;

	battery_charging_control(CHARGING_CMD_SET_VINDPM, &vindpm);
	battery_log(BAT_LOG_CRTI,
		    "[PE+] switch charger set VINDPM=%dmV with charger volatge=%dmV\n",
		    vindpm * 100 + 2600, chr_v);
}
#endif

#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)
static DEFINE_MUTEX(ta_mutex);

static void set_ta_charging_current(void)
{
	int real_v_chrA = 0;

	real_v_chrA = battery_meter_get_charger_voltage();
#if defined(TA_AC_12V_INPUT_CURRENT)
	if ((real_v_chrA - ta_v_chr_org) > 6000) {
		g_temp_input_CC_value = TA_AC_12V_INPUT_CURRENT;	/* CHARGE_CURRENT_2000_00_MA */
		g_temp_CC_value = batt_cust_data.ta_ac_charging_current;
	} else
#endif
	if ((real_v_chrA - ta_v_chr_org) > 3000) {
		g_temp_input_CC_value = batt_cust_data.ta_ac_9v_input_current;	/* TA = 9V */
		g_temp_CC_value = batt_cust_data.ta_ac_charging_current;
	} else if ((real_v_chrA - ta_v_chr_org) > 1000) {
		g_temp_input_CC_value = batt_cust_data.ta_ac_7v_input_current;	/* TA = 7V */
		g_temp_CC_value = batt_cust_data.ta_ac_charging_current;
	}
	battery_log(BAT_LOG_CRTI, "[PE+]set Ichg=%dmA with Iinlim=%dmA, chrA=%d, chrB=%d\n",
		    g_temp_CC_value / 100, g_temp_input_CC_value / 100, ta_v_chr_org, real_v_chrA);
}

static void mtk_ta_reset_vchr(void)
{
	CHR_CURRENT_ENUM chr_current = CHARGE_CURRENT_70_00_MA;

	battery_charging_control(CHARGING_CMD_SET_INPUT_CURRENT, &chr_current);
	msleep(250);		/* reset Vchr to 5V */

	battery_log(BAT_LOG_CRTI, "[PE+]mtk_ta_reset_vchr(): reset Vchr to 5V\n");
}

static void mtk_ta_increase(void)
{
	kal_bool ta_current_pattern = KAL_TRUE;	/* TRUE = increase */

	if (ta_cable_out_occur == KAL_FALSE) {
		battery_charging_control(CHARGING_CMD_SET_TA_CURRENT_PATTERN, &ta_current_pattern);
	} else {
		ta_check_chr_type = KAL_TRUE;
		battery_log(BAT_LOG_CRTI, "[PE+]mtk_ta_increase() Cable out\n");
	}
}

static kal_bool mtk_ta_retry_increase(void)
{
	int real_v_chrA;
	int real_v_chrB;
	kal_bool retransmit = KAL_TRUE;
	unsigned int retransmit_count = 0;

	do {
		real_v_chrA = battery_meter_get_charger_voltage();
		mtk_ta_increase();	/* increase TA voltage to 7V */
		real_v_chrB = battery_meter_get_charger_voltage();

		if (real_v_chrB - real_v_chrA >= 1000) /* 1.0V */
			retransmit = KAL_FALSE;
		else {
			retransmit_count++;
			battery_log(BAT_LOG_CRTI,
				    "[PE+]Communicated with adapter:retransmit_count =%d, chrA=%d, chrB=%d\n",
				    retransmit_count, real_v_chrA, real_v_chrB);
		}

		if ((retransmit_count == 3) || (BMT_status.charger_exist == KAL_FALSE))
			retransmit = KAL_FALSE;


	} while ((retransmit == KAL_TRUE) && (ta_cable_out_occur == KAL_FALSE));

	battery_log(BAT_LOG_CRTI,
		    "[PE+]Finished communicating with adapter real_v_chrA=%d, real_v_chrB=%d, retry=%d\n",
		    real_v_chrA, real_v_chrB, retransmit_count);

	if (retransmit_count == 3)
		return KAL_FALSE;

		return KAL_TRUE;
}

static void mtk_ta_detector(void)
{
	int real_v_chrB = 0;
#if defined(CONFIG_MTK_BQ25896_SUPPORT)
	/*unsigned int chr_ovp;*/
	unsigned int vindpm;
	unsigned int hw_ovp_en;

	/*Need to disable CHR_VCDT_HV before pump to 7V */
	/*chr_ovp = TA_AC_7V_INPUT_OVER_VOLTAGE;
	battery_charging_control(CHARGING_CMD_SET_HV_THRESHOLD, &chr_ovp);*/
	hw_ovp_en = 0;
	battery_charging_control(CHARGING_CMD_SET_VBUS_OVP_EN, &hw_ovp_en);
	batt_cust_data.v_charger_max = 15000;
#endif
	battery_log(BAT_LOG_CRTI, "[PE+]starting PE+ adapter detection\n");

	ta_v_chr_org = battery_meter_get_charger_voltage();
	mtk_ta_retry_increase();
	real_v_chrB = battery_meter_get_charger_voltage();

	if (real_v_chrB - ta_v_chr_org >= 1000)
		is_ta_connect = KAL_TRUE;
	else {
		is_ta_connect = KAL_FALSE;
#if defined(CONFIG_MTK_BQ25896_SUPPORT)
		/*chr_ovp = TA_AC_5V_INPUT_OVER_VOLTAGE;
		battery_charging_control(CHARGING_CMD_SET_HV_THRESHOLD, &chr_ovp);*/
		hw_ovp_en = 1;
		battery_charging_control(CHARGING_CMD_SET_VBUS_OVP_EN, &hw_ovp_en);
		batt_cust_data.v_charger_max = V_CHARGER_MAX;

		/*Set BQ25896 VINDPM to 4.6V for vbus = 5V */
		vindpm = SWITCH_CHR_VINDPM_5V;
		battery_charging_control(CHARGING_CMD_SET_VINDPM, &vindpm);
#endif
	}

	battery_log(BAT_LOG_CRTI, "[PE+]End of PE+ adapter detection, is_ta_connect=%d\n",
		    is_ta_connect);
}

static void mtk_ta_init(void)
{
	is_ta_connect = KAL_FALSE;
	ta_cable_out_occur = KAL_FALSE;

	if (batt_cust_data.ta_9v_support || batt_cust_data.ta_12v_support)
		ta_vchr_tuning = KAL_FALSE;

	battery_charging_control(CHARGING_CMD_INIT, NULL);
}

static void battery_pump_express_charger_check(void)
{
	if (KAL_TRUE == ta_check_chr_type &&
	    STANDARD_CHARGER == BMT_status.charger_type &&
	    BMT_status.SOC >= batt_cust_data.ta_start_battery_soc &&
	    BMT_status.SOC < batt_cust_data.ta_stop_battery_soc) {
		battery_log(BAT_LOG_CRTI, "[PE+]Starting PE Adaptor detection\n");

		mutex_lock(&ta_mutex);
		wake_lock(&TA_charger_suspend_lock);

		mtk_ta_reset_vchr();

		mtk_ta_init();
		mtk_ta_detector();

		/* need to re-check if the charger plug out during ta detector */
		if (KAL_TRUE == ta_cable_out_occur)
			ta_check_chr_type = KAL_TRUE;
		else
			ta_check_chr_type = KAL_FALSE;
#if defined(PUMPEX_PLUS_RECHG)
		/*PE detection disable in the event of recharge after 1st PE detection is finished */
		pep_det_rechg = KAL_FALSE;
#endif
		wake_unlock(&TA_charger_suspend_lock);
		mutex_unlock(&ta_mutex);
	} else {
		battery_log(BAT_LOG_CRTI,
			    "[PE+]Stop battery_pump_express_charger_check, SOC=%d, ta_check_chr_type = %d, charger_type = %d\n",
			    BMT_status.SOC, ta_check_chr_type, BMT_status.charger_type);
	}
}

static void battery_pump_express_algorithm_start(void)
{
	signed int charger_vol;
	unsigned int charging_enable = KAL_FALSE;

#if defined(CONFIG_MTK_DYNAMIC_BAT_CV_SUPPORT)
	unsigned int cv;
	unsigned int vbat;
#endif
#if defined(TA_12V_SUPPORT)
	kal_bool pumped_volt;
	unsigned int chr_ovp_en;

	battery_log(BAT_LOG_CRTI, "[PE+][battery_pump_express_algorithm_start]start PEP...");
#endif

	mutex_lock(&ta_mutex);
	wake_lock(&TA_charger_suspend_lock);

	if (KAL_TRUE == is_ta_connect) {
		/* check cable impedance */
		charger_vol = battery_meter_get_charger_voltage();
		batt_cust_data.v_charger_max = 15000;
#if defined(CONFIG_MTK_DYNAMIC_BAT_CV_SUPPORT)
		cv = get_constant_voltage();
		vbat = battery_meter_get_battery_voltage(KAL_TRUE);
#endif
		if (KAL_FALSE == ta_vchr_tuning) {
#ifndef TA_12V_SUPPORT
			mtk_ta_retry_increase();
			charger_vol = battery_meter_get_charger_voltage();

#else
			/*1. Set CHR_HV to a higher level than 9V */
			/*chr_ovp = TA_AC_9V_INPUT_OVER_VOLTAGE;
			battery_charging_control(CHARGING_CMD_SET_HV_THRESHOLD, &chr_ovp);*/

			/* increase TA voltage to 9V */
			pumped_volt = mtk_ta_retry_increase();

			if (pumped_volt == KAL_FALSE) {
				battery_log(BAT_LOG_CRTI,
					    "[PE+]adaptor failed to output 9V, Please check adaptor");
				/*1. Set BQ25896 VINDPM back to 6.3V for Vbus = 7V */
				/* vindpm = SWITCH_CHR_VINDPM_7V; */
				/* battery_charging_control(CHARGING_CMD_SET_VINDPM, &vindpm); */
				/*2. Set CHR_HV back to a higher level than 7V */
				/*chr_ovp = TA_AC_7V_INPUT_OVER_VOLTAGE;
				battery_charging_control(CHARGING_CMD_SET_HV_THRESHOLD, &chr_ovp);*/
			}
#endif
#if defined(TA_12V_SUPPORT)
			else {
				/*1. disable PMIC VBUS OVP : VCDT */
				chr_ovp_en = 0;
				battery_charging_control(CHARGING_CMD_SET_VBUS_OVP_EN, &chr_ovp_en);
				batt_cust_data.v_charger_max = 15000;

				/*2. Set BQ25896 VINDPM to 10.5V for vbus = 12V */
				/* vindpm = SWITCH_CHR_VINDPM_12V; */
				/* battery_charging_control(CHARGING_CMD_SET_VINDPM, &vindpm); */

				/*ready to pump up to 12V */
				pumped_volt = mtk_ta_retry_increase();	/* increase TA voltage to 12V */
				if (pumped_volt == KAL_FALSE) {
					/*1. Enable PMIC VBUS OVP : VCDT */
					chr_ovp_en = 0;
					battery_charging_control(CHARGING_CMD_SET_VBUS_OVP_EN,
								 &chr_ovp_en);

					/*2. Reset BQ25896 VINDPM back to 8.1V */
					/* vindpm = SWITCH_CHR_VINDPM_9V; */
					/* battery_charging_control(CHARGING_CMD_SET_VINDPM, &vindpm); */

					battery_log(BAT_LOG_CRTI,
						    "[PE+]adaptor failed to output 12V, Please check adaptor.");
				} else
					battery_log(BAT_LOG_FULL,
						    "[PE+]adaptor successed to output 12V.");
				charger_vol = battery_meter_get_charger_voltage();
			}
#endif

			ta_vchr_tuning = KAL_TRUE;
		} else if (BMT_status.SOC > batt_cust_data.ta_stop_battery_soc) {
			/* disable charging, avoid Iterm issue */
			battery_charging_control(CHARGING_CMD_ENABLE, &charging_enable);
			mtk_ta_reset_vchr();	/* decrease TA voltage to 5V */
			charger_vol = battery_meter_get_charger_voltage();
			if (abs(charger_vol - ta_v_chr_org) <= 1000)	/* 1.0V */
				is_ta_connect = KAL_FALSE;

			battery_log(BAT_LOG_CRTI,
				    "[PE+]Stop battery_pump_express_algorithm, SOC=%d is_ta_connect =%d, TA_STOP_BATTERY_SOC: %d\n",
				    BMT_status.SOC, is_ta_connect, batt_cust_data.ta_stop_battery_soc);
		}
#if defined(CONFIG_MTK_DYNAMIC_BAT_CV_SUPPORT)
/*For BQ25896, voltage is used to check if PE+ should be tuned off.*/
		else if (vbat >= cv) {
			/*CV point reached, disable PE+ */
			battery_charging_control(CHARGING_CMD_ENABLE, &charging_enable);
			mtk_ta_reset_vchr();	/* decrease TA voltage to 5V */
			charger_vol = battery_meter_get_charger_voltage();
			if (abs(charger_vol - ta_v_chr_org) <= 1000)	/* 1.0V */
				is_ta_connect = KAL_FALSE;

			/*1. Recover CHR_OVP status */
			/*chr_ovp = TA_AC_5V_INPUT_OVER_VOLTAGE;
			battery_charging_control(CHARGING_CMD_SET_HV_THRESHOLD, &chr_ovp);*/
			chr_ovp_en = 1;
			battery_charging_control(CHARGING_CMD_SET_VBUS_OVP_EN, &chr_ovp_en);

			/*2. Recover SW_OVP status */
			batt_cust_data.v_charger_max = V_CHARGER_MAX;
			battery_log(BAT_LOG_CRTI,
				    "[PE+]CV=%d reached. Stopping PE+, is_ta_connect =%d\n", cv,
				    is_ta_connect);
		} else if (0) {
			/*check charger voltage status if vbus is dropped*/
			if (abs(charger_vol - ta_v_chr_org) < 1000 && BMT_status.bat_charging_state == CHR_CC) {
				ta_check_chr_type = KAL_TRUE;
				battery_log(BAT_LOG_CRTI, "[PE+] abnormal TA chager voltage, rechecking PE+ adapter\n");
			}
		}
		/*Set VINDPM after Vbus voltage is set or reset */
		switch_charger_set_vindpm(charger_vol);

#endif
		battery_log(BAT_LOG_CRTI,
			    "[PE+]check cable impedance, VA(%d) VB(%d) delta(%d).\n",
			    ta_v_chr_org, charger_vol, charger_vol - ta_v_chr_org);

		battery_log(BAT_LOG_CRTI, "[PE+]mtk_ta_algorithm() end\n");
	} else {
		battery_log(BAT_LOG_CRTI, "[PE+]Not a TA charger, bypass TA algorithm\n");
#if defined(TA_12V_SUPPORT)
		batt_cust_data.v_charger_max = V_CHARGER_MAX;
		chr_ovp_en = 1;
		battery_charging_control(CHARGING_CMD_SET_VBUS_OVP_EN, &chr_ovp_en);
#endif
	}

	wake_unlock(&TA_charger_suspend_lock);
	mutex_unlock(&ta_mutex);
}
#endif

#if defined(CONFIG_MTK_JEITA_STANDARD_SUPPORT)

static BATTERY_VOLTAGE_ENUM select_jeita_cv(void)
{
	BATTERY_VOLTAGE_ENUM cv_voltage;

	if (g_temp_status == TEMP_ABOVE_POS_60) {
		cv_voltage = JEITA_TEMP_ABOVE_POS_60_CV_VOLTAGE;
	} else if (g_temp_status == TEMP_POS_45_TO_POS_60) {
		cv_voltage = JEITA_TEMP_POS_45_TO_POS_60_CV_VOLTAGE;
	} else if (g_temp_status == TEMP_POS_10_TO_POS_45) {
		if (batt_cust_data.high_battery_voltage_support)
			cv_voltage = BATTERY_VOLT_04_340000_V;
		else
			cv_voltage = JEITA_TEMP_POS_10_TO_POS_45_CV_VOLTAGE;
	} else if (g_temp_status == TEMP_POS_0_TO_POS_10) {
		cv_voltage = JEITA_TEMP_POS_0_TO_POS_10_CV_VOLTAGE;
	} else if (g_temp_status == TEMP_NEG_10_TO_POS_0) {
		cv_voltage = JEITA_TEMP_NEG_10_TO_POS_0_CV_VOLTAGE;
	} else if (g_temp_status == TEMP_BELOW_NEG_10) {
		cv_voltage = JEITA_TEMP_BELOW_NEG_10_CV_VOLTAGE;
	} else {
		cv_voltage = BATTERY_VOLT_04_200000_V;
	}

	return cv_voltage;
}

PMU_STATUS do_jeita_state_machine(void)
{
	BATTERY_VOLTAGE_ENUM cv_voltage;
	PMU_STATUS jeita_status = PMU_STATUS_OK;

	/* JEITA battery temp Standard */

	if (BMT_status.temperature >= TEMP_POS_60_THRESHOLD) {
		battery_log(BAT_LOG_CRTI,
			    "[BATTERY] Battery Over high Temperature(%d) !!\n\r",
			    TEMP_POS_60_THRESHOLD);

		g_temp_status = TEMP_ABOVE_POS_60;

		return PMU_STATUS_FAIL;
	} else if (BMT_status.temperature > TEMP_POS_45_THRESHOLD) {	/* control 45c to normal behavior */
		if ((g_temp_status == TEMP_ABOVE_POS_60)
		    && (BMT_status.temperature >= TEMP_POS_60_THRES_MINUS_X_DEGREE)) {
			battery_log(BAT_LOG_CRTI,
				    "[BATTERY] Battery Temperature between %d and %d,not allow charging yet!!\n\r",
				    TEMP_POS_60_THRES_MINUS_X_DEGREE, TEMP_POS_60_THRESHOLD);

			jeita_status = PMU_STATUS_FAIL;
		} else {
			battery_log(BAT_LOG_CRTI,
				    "[BATTERY] Battery Temperature between %d and %d !!\n\r",
				    TEMP_POS_45_THRESHOLD, TEMP_POS_60_THRESHOLD);

			g_temp_status = TEMP_POS_45_TO_POS_60;
		}
	} else if (BMT_status.temperature >= TEMP_POS_10_THRESHOLD) {
		if (((g_temp_status == TEMP_POS_45_TO_POS_60)
		     && (BMT_status.temperature >= TEMP_POS_45_THRES_MINUS_X_DEGREE))
		    || ((g_temp_status == TEMP_POS_0_TO_POS_10)
			&& (BMT_status.temperature <= TEMP_POS_10_THRES_PLUS_X_DEGREE))) {
			battery_log(BAT_LOG_CRTI,
				    "[BATTERY] Battery Temperature not recovery to normal temperature charging mode yet!!\n\r");
		} else {
			battery_log(BAT_LOG_CRTI,
				    "[BATTERY] Battery Normal Temperature between %d and %d !!\n\r",
				    TEMP_POS_10_THRESHOLD, TEMP_POS_45_THRESHOLD);
			g_temp_status = TEMP_POS_10_TO_POS_45;
		}
	} else if (BMT_status.temperature >= TEMP_POS_0_THRESHOLD) {
		if ((g_temp_status == TEMP_NEG_10_TO_POS_0 || g_temp_status == TEMP_BELOW_NEG_10)
		    && (BMT_status.temperature <= TEMP_POS_0_THRES_PLUS_X_DEGREE)) {
			if (g_temp_status == TEMP_NEG_10_TO_POS_0) {
				battery_log(BAT_LOG_CRTI,
					    "[BATTERY] Battery Temperature between %d and %d !!\n\r",
					    TEMP_POS_0_THRES_PLUS_X_DEGREE, TEMP_POS_10_THRESHOLD);
			}
			if (g_temp_status == TEMP_BELOW_NEG_10) {
				battery_log(BAT_LOG_CRTI,
					    "[BATTERY] Battery Temperature between %d and %d,not allow charging yet!!\n\r",
					    TEMP_POS_0_THRESHOLD, TEMP_POS_0_THRES_PLUS_X_DEGREE);
				return PMU_STATUS_FAIL;
			}
		} else {
			battery_log(BAT_LOG_CRTI,
				    "[BATTERY] Battery Temperature between %d and %d !!\n\r",
				    TEMP_POS_0_THRESHOLD, TEMP_POS_10_THRESHOLD);

			g_temp_status = TEMP_POS_0_TO_POS_10;
		}
	} else if (BMT_status.temperature >= TEMP_NEG_10_THRESHOLD) {
		if ((g_temp_status == TEMP_BELOW_NEG_10)
		    && (BMT_status.temperature <= TEMP_NEG_10_THRES_PLUS_X_DEGREE)) {
			battery_log(BAT_LOG_CRTI,
				    "[BATTERY] Battery Temperature between %d and %d,not allow charging yet!!\n\r",
				    TEMP_NEG_10_THRESHOLD, TEMP_NEG_10_THRES_PLUS_X_DEGREE);

			jeita_status = PMU_STATUS_FAIL;
		} else {
			battery_log(BAT_LOG_CRTI,
				    "[BATTERY] Battery Temperature between %d and %d !!\n\r",
				    TEMP_NEG_10_THRESHOLD, TEMP_POS_0_THRESHOLD);

			g_temp_status = TEMP_NEG_10_TO_POS_0;
		}
	} else {
		battery_log(BAT_LOG_CRTI,
			    "[BATTERY] Battery below low Temperature(%d) !!\n\r",
			    TEMP_NEG_10_THRESHOLD);
		g_temp_status = TEMP_BELOW_NEG_10;

		jeita_status = PMU_STATUS_FAIL;
	}

	/* set CV after temperature changed */

	cv_voltage = select_jeita_cv();
	battery_charging_control(CHARGING_CMD_SET_CV_VOLTAGE, &cv_voltage);

#if defined(CONFIG_MTK_HAFG_20)
	g_cv_voltage = cv_voltage;
#endif

	return jeita_status;
}


static void set_jeita_charging_current(void)
{
#ifdef CONFIG_USB_IF
	if (BMT_status.charger_type == STANDARD_HOST)
		return;
#endif

	if (g_temp_status == TEMP_NEG_10_TO_POS_0) {
		g_temp_CC_value = CHARGE_CURRENT_350_00_MA;
		g_temp_input_CC_value = CHARGE_CURRENT_500_00_MA;
		battery_log(BAT_LOG_CRTI, "[BATTERY] JEITA set charging current : %d\r\n",
			    g_temp_CC_value);
	}
}

#endif

bool get_usb_current_unlimited(void)
{
	if (BMT_status.charger_type == STANDARD_HOST || BMT_status.charger_type == CHARGING_HOST)
		return usb_unlimited;

		return false;
}

void set_usb_current_unlimited(bool enable)
{
	unsigned int en;
	usb_unlimited = enable;

	if (enable == true)
		en = 1;
	else
		en = 0;
	battery_charging_control(CHARGING_CMD_ENABLE_SAFETY_TIMER, &en);
}

void select_charging_current_bcct(void)
{
/*BQ25896 is the first switch chrager separating input and charge current
* any switch charger can use this compile option which may be generalized
* to be CONFIG_SWITCH_INPUT_OUTPUT_CURRENT_SUPPORT
*/
#ifndef CONFIG_MTK_SWITCH_INPUT_OUTPUT_CURRENT_SUPPORT
	if ((BMT_status.charger_type == STANDARD_HOST) ||
	    (BMT_status.charger_type == NONSTANDARD_CHARGER)) {
		if (g_bcct_value < 100)
			g_temp_input_CC_value = CHARGE_CURRENT_0_00_MA;
		else if (g_bcct_value < 500)
			g_temp_input_CC_value = CHARGE_CURRENT_100_00_MA;
		else if (g_bcct_value < 800)
			g_temp_input_CC_value = CHARGE_CURRENT_500_00_MA;
		else if (g_bcct_value == 800)
			g_temp_input_CC_value = CHARGE_CURRENT_800_00_MA;
		else
			g_temp_input_CC_value = CHARGE_CURRENT_500_00_MA;
	} else if ((BMT_status.charger_type == STANDARD_CHARGER) ||
		   (BMT_status.charger_type == CHARGING_HOST)) {
		g_temp_input_CC_value = CHARGE_CURRENT_MAX;

		/* --------------------------------------------------- */
		/* set IOCHARGE */
		if (g_bcct_value < 550)
			g_temp_CC_value = CHARGE_CURRENT_0_00_MA;
		else if (g_bcct_value < 650)
			g_temp_CC_value = CHARGE_CURRENT_550_00_MA;
		else if (g_bcct_value < 750)
			g_temp_CC_value = CHARGE_CURRENT_650_00_MA;
		else if (g_bcct_value < 850)
			g_temp_CC_value = CHARGE_CURRENT_750_00_MA;
		else if (g_bcct_value < 950)
			g_temp_CC_value = CHARGE_CURRENT_850_00_MA;
		else if (g_bcct_value < 1050)
			g_temp_CC_value = CHARGE_CURRENT_950_00_MA;
		else if (g_bcct_value < 1150)
			g_temp_CC_value = CHARGE_CURRENT_1050_00_MA;
		else if (g_bcct_value < 1250)
			g_temp_CC_value = CHARGE_CURRENT_1150_00_MA;
		else if (g_bcct_value == 1250)
			g_temp_CC_value = CHARGE_CURRENT_1250_00_MA;
		else
			g_temp_CC_value = CHARGE_CURRENT_650_00_MA;
		/* --------------------------------------------------- */

	} else {
		g_temp_input_CC_value = CHARGE_CURRENT_500_00_MA;
	}
#else
	if (g_bcct_flag == 1)
		g_temp_CC_value = g_bcct_value * 100;
	if (g_bcct_input_flag == 1)
		g_temp_input_CC_value = g_bcct_input_value * 100;
/*
		if ((BMT_status.charger_type == STANDARD_CHARGER) ||
		   (BMT_status.charger_type == CHARGING_HOST)) {
		if (g_bcct_value < 550)
			g_temp_CC_value = CHARGE_CURRENT_550_00_MA;
		else
			g_temp_CC_value = g_bcct_value * 100;

		if (g_bcct_value < 650)
			g_temp_CC_value = CHARGE_CURRENT_550_00_MA;
		else if (g_bcct_value < 750)
			g_temp_CC_value = CHARGE_CURRENT_650_00_MA;
		else if (g_bcct_value < 850)
			g_temp_CC_value = CHARGE_CURRENT_750_00_MA;
		else if (g_bcct_value < 950)
			g_temp_CC_value = CHARGE_CURRENT_850_00_MA;
		else if (g_bcct_value < 1050)
			g_temp_CC_value = CHARGE_CURRENT_950_00_MA;
		else if (g_bcct_value < 1150)
			g_temp_CC_value = CHARGE_CURRENT_1050_00_MA;
		else if (g_bcct_value < 1250)
			g_temp_CC_value = CHARGE_CURRENT_1150_00_MA;
		else if (g_bcct_value == 1250)
			g_temp_CC_value = CHARGE_CURRENT_1250_00_MA;
		else if (g_bcct_value == 1350)
			g_temp_CC_value = CHARGE_CURRENT_1350_00_MA;
		else if (g_bcct_value == 1450)
			g_temp_CC_value = CHARGE_CURRENT_1450_00_MA;
		else if (g_bcct_value == 1550)
			g_temp_CC_value = CHARGE_CURRENT_1550_00_MA;
		else if (g_bcct_value == 1650)
			g_temp_CC_value = CHARGE_CURRENT_1650_00_MA;
		else if (g_bcct_value == 1750)
			g_temp_CC_value = CHARGE_CURRENT_1750_00_MA;
		else if (g_bcct_value == 1850)
			g_temp_CC_value = CHARGE_CURRENT_1850_00_MA;
		else if (g_bcct_value == 1950)
			g_temp_CC_value = CHARGE_CURRENT_1950_00_MA;
		else if (g_bcct_value == 2050)
			g_temp_CC_value = CHARGE_CURRENT_2050_00_MA;
		else if (g_bcct_value == 2150)
			g_temp_CC_value = CHARGE_CURRENT_2150_00_MA;
		else if (g_bcct_value == 2250)
			g_temp_CC_value = CHARGE_CURRENT_2250_00_MA;
		else if (g_bcct_value == 2350)
			g_temp_CC_value = CHARGE_CURRENT_2350_00_MA;
		else if (g_bcct_value == 2450)
			g_temp_CC_value = CHARGE_CURRENT_2450_00_MA;
		else if (g_bcct_value == 2550)
			g_temp_CC_value = CHARGE_CURRENT_2550_00_MA;
		else if (g_bcct_value == 2650)
			g_temp_CC_value = CHARGE_CURRENT_2650_00_MA;
		else if (g_bcct_value == 2750)
			g_temp_CC_value = CHARGE_CURRENT_2750_00_MA;
		else if (g_bcct_value == 2850)
			g_temp_CC_value = CHARGE_CURRENT_2850_00_MA;
		else if (g_bcct_value == 2950)
			g_temp_CC_value = CHARGE_CURRENT_2950_00_MA;
		else
			g_temp_CC_value = CHARGE_CURRENT_650_00_MA;

	} else {
		g_temp_CC_value = CHARGE_CURRENT_500_00_MA;
	}
*/
#endif
}


/*BQ25896 is the first switch chrager separating input and charge current
*/
unsigned int set_chr_input_current_limit(int current_limit)
{
#ifdef CONFIG_MTK_SWITCH_INPUT_OUTPUT_CURRENT_SUPPORT
	if (current_limit != -1) {
		g_bcct_input_flag = 1;

		if ((BMT_status.charger_type == STANDARD_HOST) ||
		    (BMT_status.charger_type == NONSTANDARD_CHARGER)) {
				g_temp_input_CC_value = CHARGE_CURRENT_500_00_MA;
				g_bcct_input_value = CHARGE_CURRENT_500_00_MA / 100;
		} else if ((BMT_status.charger_type == STANDARD_CHARGER) ||
			(BMT_status.charger_type == CHARGING_HOST)) {

			if (current_limit < 650) {
				g_temp_input_CC_value = CHARGE_CURRENT_650_00_MA;
				g_bcct_input_value = CHARGE_CURRENT_650_00_MA / 100;
			} else {
				g_temp_input_CC_value = current_limit * 100;
				g_bcct_input_value = current_limit;
			}
		} else {
			g_temp_input_CC_value = CHARGE_CURRENT_500_00_MA;
			g_bcct_input_value = CHARGE_CURRENT_500_00_MA / 100;
		}

		battery_log(BAT_LOG_CRTI, "[BATTERY] set_chr_input_current_limit (%d)\r\n",
		current_limit);
	} else {
		/* change to default current setting */
		g_bcct_input_flag = 0;
}

	/* wake_up_bat(); */
	/*pchr_turn_on_charging();*/
	/* this function must be followed by set_bat_charging_current_limit()*/
	return g_bcct_input_flag;
#else
	battery_log(BAT_LOG_CRTI, "[BATTERY] set_chr_input_current_limit _NOT_ supported\n");
	return 0;
#endif
}


static void pchr_turn_on_charging(void);
unsigned int set_bat_charging_current_limit(int current_limit)
{
	battery_log(BAT_LOG_CRTI, "[BATTERY] set_bat_charging_current_limit (%d)\r\n",
		    current_limit);

	if (current_limit != -1) {
		g_bcct_flag = 1;
		g_bcct_value = current_limit;
#ifdef CONFIG_MTK_THERMAL_TEST_SUPPORT
		g_temp_CC_value = current_limit * 100;
#else
		if (current_limit < 70)
			g_temp_CC_value = CHARGE_CURRENT_0_00_MA;
		else if (current_limit < 200)
			g_temp_CC_value = CHARGE_CURRENT_70_00_MA;
		else if (current_limit < 300)
			g_temp_CC_value = CHARGE_CURRENT_200_00_MA;
		else if (current_limit < 400)
			g_temp_CC_value = CHARGE_CURRENT_300_00_MA;
		else if (current_limit < 450)
			g_temp_CC_value = CHARGE_CURRENT_400_00_MA;
		else if (current_limit < 550)
			g_temp_CC_value = CHARGE_CURRENT_450_00_MA;
		else if (current_limit < 650)
			g_temp_CC_value = CHARGE_CURRENT_550_00_MA;
		else if (current_limit < 700)
			g_temp_CC_value = CHARGE_CURRENT_650_00_MA;
		else if (current_limit < 800)
			g_temp_CC_value = CHARGE_CURRENT_700_00_MA;
		else if (current_limit < 900)
			g_temp_CC_value = CHARGE_CURRENT_800_00_MA;
		else if (current_limit < 1000)
			g_temp_CC_value = CHARGE_CURRENT_900_00_MA;
		else if (current_limit < 1100)
			g_temp_CC_value = CHARGE_CURRENT_1000_00_MA;
		else if (current_limit < 1200)
			g_temp_CC_value = CHARGE_CURRENT_1100_00_MA;
		else if (current_limit < 1300)
			g_temp_CC_value = CHARGE_CURRENT_1200_00_MA;
		else if (current_limit < 1400)
			g_temp_CC_value = CHARGE_CURRENT_1300_00_MA;
		else if (current_limit < 1500)
			g_temp_CC_value = CHARGE_CURRENT_1400_00_MA;
		else if (current_limit < 1600)
			g_temp_CC_value = CHARGE_CURRENT_1500_00_MA;
		else if (current_limit == 1600)
			g_temp_CC_value = CHARGE_CURRENT_1600_00_MA;
		else
			g_temp_CC_value = CHARGE_CURRENT_450_00_MA;
#endif
	} else {
		/* change to default current setting */
		g_bcct_flag = 0;
	}

	/* wake_up_bat(); */
	pchr_turn_on_charging();

	return g_bcct_flag;
}


void select_charging_current(void)
{
	if (g_ftm_battery_flag) {
		battery_log(BAT_LOG_CRTI, "[BATTERY] FTM charging : %d\r\n",
			    charging_level_data[0]);
		g_temp_CC_value = charging_level_data[0];

		if (g_temp_CC_value == CHARGE_CURRENT_450_00_MA) {
			g_temp_input_CC_value = CHARGE_CURRENT_500_00_MA;
		} else {
			g_temp_input_CC_value = CHARGE_CURRENT_MAX;
			g_temp_CC_value = batt_cust_data.ac_charger_current;

			battery_log(BAT_LOG_CRTI, "[BATTERY] set_ac_current \r\n");
		}
	} else {
		if (BMT_status.charger_type == STANDARD_HOST) {
#ifdef CONFIG_USB_IF
			{
				g_temp_input_CC_value = CHARGE_CURRENT_MAX;
				if (g_usb_state == USB_SUSPEND)
					g_temp_CC_value = USB_CHARGER_CURRENT_SUSPEND;
				else if (g_usb_state == USB_UNCONFIGURED)
					g_temp_CC_value = batt_cust_data.usb_charger_current_unconfigured;
				else if (g_usb_state == USB_CONFIGURED)
					g_temp_CC_value = batt_cust_data.usb_charger_current_configured;
				else
					g_temp_CC_value = batt_cust_data.usb_charger_current_unconfigured;

				battery_log(BAT_LOG_CRTI,
					    "[BATTERY] STANDARD_HOST CC mode charging : %d on %d state\r\n",
					    g_temp_CC_value, g_usb_state);
			}
#else
			{
				g_temp_input_CC_value = batt_cust_data.usb_charger_current;
				g_temp_CC_value = batt_cust_data.usb_charger_current;
			}
#endif
		} else if (BMT_status.charger_type == NONSTANDARD_CHARGER) {
			g_temp_input_CC_value = batt_cust_data.non_std_ac_charger_current;
			g_temp_CC_value = batt_cust_data.non_std_ac_charger_current;

		} else if (BMT_status.charger_type == STANDARD_CHARGER) {
			if (batt_cust_data.ac_charger_input_current != 0)
				g_temp_input_CC_value = batt_cust_data.ac_charger_input_current;
			else
				g_temp_input_CC_value = batt_cust_data.ac_charger_current;

			g_temp_CC_value = batt_cust_data.ac_charger_current;
#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)
			if (is_ta_connect == KAL_TRUE)
				set_ta_charging_current();
#endif
		} else if (BMT_status.charger_type == CHARGING_HOST) {
			g_temp_input_CC_value = batt_cust_data.charging_host_charger_current;
			g_temp_CC_value = batt_cust_data.charging_host_charger_current;
		} else if (BMT_status.charger_type == APPLE_2_1A_CHARGER) {
			g_temp_input_CC_value = batt_cust_data.apple_2_1a_charger_current;
			g_temp_CC_value = batt_cust_data.apple_2_1a_charger_current;
		} else if (BMT_status.charger_type == APPLE_1_0A_CHARGER) {
			g_temp_input_CC_value = batt_cust_data.apple_1_0a_charger_current;
			g_temp_CC_value = batt_cust_data.apple_1_0a_charger_current;
		} else if (BMT_status.charger_type == APPLE_0_5A_CHARGER) {
			g_temp_input_CC_value = batt_cust_data.apple_0_5a_charger_current;
			g_temp_CC_value = batt_cust_data.apple_0_5a_charger_current;
		} else {
			g_temp_input_CC_value = CHARGE_CURRENT_500_00_MA;
			g_temp_CC_value = CHARGE_CURRENT_500_00_MA;
		}

#if defined(CONFIG_MTK_DUAL_INPUT_CHARGER_SUPPORT)
		if (DISO_data.diso_state.cur_vdc_state == DISO_ONLINE) {
			g_temp_input_CC_value = batt_cust_data.ac_charger_current;
			g_temp_CC_value = batt_cust_data.ac_charger_current;
		}
#endif

#if defined(CONFIG_MTK_JEITA_STANDARD_SUPPORT)
		set_jeita_charging_current();
#endif
	}

}

static unsigned int charging_full_check(void)
{
	unsigned int status;

	battery_charging_control(CHARGING_CMD_GET_CHARGING_STATUS, &status);
	if (status == KAL_TRUE) {
		g_full_check_count++;
		if (g_full_check_count >= FULL_CHECK_TIMES)
			return KAL_TRUE;
		else
			return KAL_FALSE;
	} /*else {*/
		g_full_check_count = 0;
		return status;
	/*}*/
}


static void pchr_turn_on_charging(void)
{
#if !defined(CONFIG_MTK_JEITA_STANDARD_SUPPORT)
	BATTERY_VOLTAGE_ENUM cv_voltage;
#endif
	unsigned int charging_enable = KAL_TRUE;

#if defined(CONFIG_MTK_DUAL_INPUT_CHARGER_SUPPORT)
	if (KAL_TRUE == BMT_status.charger_exist)
		charging_enable = KAL_TRUE;
	else
		charging_enable = KAL_FALSE;
#endif

	if (BMT_status.bat_charging_state == CHR_ERROR) {
		battery_log(BAT_LOG_CRTI, "[BATTERY] Charger Error, turn OFF charging !\n");

		charging_enable = KAL_FALSE;

	} else if ((g_platform_boot_mode == META_BOOT) || (g_platform_boot_mode == ADVMETA_BOOT)) {
		battery_log(BAT_LOG_CRTI,
			    "[BATTERY] In meta or advanced meta mode, disable charging.\n");
		charging_enable = KAL_FALSE;
	} else {
		/*HW initialization */
		battery_charging_control(CHARGING_CMD_INIT, NULL);

		battery_log(BAT_LOG_FULL, "charging_hw_init\n");

#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)
		battery_pump_express_algorithm_start();
#endif

		/* Set Charging Current */
		if (get_usb_current_unlimited()) {
			if (batt_cust_data.ac_charger_input_current != 0)
				g_temp_input_CC_value = batt_cust_data.ac_charger_input_current;
			else
				g_temp_input_CC_value = batt_cust_data.ac_charger_current;

			g_temp_CC_value = batt_cust_data.ac_charger_current;
			battery_log(BAT_LOG_FULL,
				    "USB_CURRENT_UNLIMITED, use batt_cust_data.ac_charger_current\n");
#ifndef CONFIG_MTK_SWITCH_INPUT_OUTPUT_CURRENT_SUPPORT
		} else if (g_bcct_flag == 1) {
			select_charging_current_bcct();

			battery_log(BAT_LOG_FULL, "[BATTERY] select_charging_current_bcct !\n");
		} else {
			select_charging_current();

			battery_log(BAT_LOG_FULL, "[BATTERY] select_charging_current !\n");
		}
#else
		} else if (g_bcct_flag == 1 || g_bcct_input_flag == 1) {
			select_charging_current();
			select_charging_current_bcct();
			battery_log(BAT_LOG_FULL, "[BATTERY] select_charging_curret_bcct !\n");
		} else {
			select_charging_current();
			battery_log(BAT_LOG_FULL, "[BATTERY] select_charging_curret !\n");
		}
#endif
		battery_log(BAT_LOG_CRTI,
			    "[BATTERY] Default CC mode charging : %d, input current = %d\r\n",
			    g_temp_CC_value, g_temp_input_CC_value);
		if (g_temp_CC_value == CHARGE_CURRENT_0_00_MA
		    || g_temp_input_CC_value == CHARGE_CURRENT_0_00_MA) {

			charging_enable = KAL_FALSE;

			battery_log(BAT_LOG_CRTI,
				    "[BATTERY] charging current is set 0mA, turn off charging !\r\n");
		} else {
			battery_charging_control(CHARGING_CMD_SET_INPUT_CURRENT,
						 &g_temp_input_CC_value);
			battery_charging_control(CHARGING_CMD_SET_CURRENT, &g_temp_CC_value);

			/*Set CV Voltage */
#if !defined(CONFIG_MTK_JEITA_STANDARD_SUPPORT)
			if (batt_cust_data.high_battery_voltage_support)
				cv_voltage = BATTERY_VOLT_04_340000_V;
			else
				cv_voltage = BATTERY_VOLT_04_200000_V;

#ifdef CONFIG_MTK_DYNAMIC_BAT_CV_SUPPORT
			cv_voltage = get_constant_voltage() * 1000;
			battery_log(BAT_LOG_CRTI, "[BATTERY][BIF] Setting CV to %d\n", cv_voltage / 1000);
			#endif
			battery_charging_control(CHARGING_CMD_SET_CV_VOLTAGE, &cv_voltage);

			#if defined(CONFIG_MTK_HAFG_20)
			g_cv_voltage = cv_voltage;
			#endif
#endif
		}
	}

	/* enable/disable charging */
	battery_charging_control(CHARGING_CMD_ENABLE, &charging_enable);

	battery_log(BAT_LOG_FULL, "[BATTERY] pchr_turn_on_charging(), enable =%d !\r\n",
		    charging_enable);
}


PMU_STATUS BAT_PreChargeModeAction(void)
{
	battery_log(BAT_LOG_CRTI, "[BATTERY] Pre-CC mode charge, timer=%d on %d !!\n\r",
		    BMT_status.PRE_charging_time, BMT_status.total_charging_time);

	BMT_status.PRE_charging_time += BAT_TASK_PERIOD;
	BMT_status.CC_charging_time = 0;
	BMT_status.TOPOFF_charging_time = 0;
	BMT_status.total_charging_time += BAT_TASK_PERIOD;

	/*  Enable charger */
	pchr_turn_on_charging();
#if defined(CONFIG_MTK_HAFG_20)
	if (BMT_status.UI_SOC2 == 100 && charging_full_check()) {
#else
	if (BMT_status.UI_SOC == 100) {
#endif
		BMT_status.bat_charging_state = CHR_BATFULL;
		BMT_status.bat_full = KAL_TRUE;
		g_charging_full_reset_bat_meter = KAL_TRUE;
	} else if (BMT_status.bat_vol > V_PRE2CC_THRES) {
		BMT_status.bat_charging_state = CHR_CC;
	}



	return PMU_STATUS_OK;
}


PMU_STATUS BAT_ConstantCurrentModeAction(void)
{
	battery_log(BAT_LOG_FULL, "[BATTERY] CC mode charge, timer=%d on %d !!\n\r",
		    BMT_status.CC_charging_time, BMT_status.total_charging_time);

	BMT_status.PRE_charging_time = 0;
	BMT_status.CC_charging_time += BAT_TASK_PERIOD;
	BMT_status.TOPOFF_charging_time = 0;
	BMT_status.total_charging_time += BAT_TASK_PERIOD;

	/*  Enable charger */
	pchr_turn_on_charging();

	if (charging_full_check() == KAL_TRUE) {
		BMT_status.bat_charging_state = CHR_BATFULL;
		BMT_status.bat_full = KAL_TRUE;
		g_charging_full_reset_bat_meter = KAL_TRUE;
	}

	return PMU_STATUS_OK;
}


PMU_STATUS BAT_BatteryFullAction(void)
{
	battery_log(BAT_LOG_CRTI, "[BATTERY] Battery full !!\n\r");

	BMT_status.bat_full = KAL_TRUE;
	BMT_status.total_charging_time = 0;
	BMT_status.PRE_charging_time = 0;
	BMT_status.CC_charging_time = 0;
	BMT_status.TOPOFF_charging_time = 0;
	BMT_status.POSTFULL_charging_time = 0;
	BMT_status.bat_in_recharging_state = KAL_FALSE;

	if (charging_full_check() == KAL_FALSE) {
		battery_log(BAT_LOG_CRTI, "[BATTERY] Battery Re-charging !!\n\r");

		BMT_status.bat_in_recharging_state = KAL_TRUE;
		BMT_status.bat_charging_state = CHR_CC;
#ifndef CONFIG_MTK_HAFG_20
		battery_meter_reset();
#endif
#if defined(PUMPEX_PLUS_RECHG) && defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)
		/*enable PE detection only once when recharge is needed */
		pep_det_rechg = KAL_TRUE;
#endif
	}


	return PMU_STATUS_OK;
}


PMU_STATUS BAT_BatteryHoldAction(void)
{
	unsigned int charging_enable;

	battery_log(BAT_LOG_CRTI, "[BATTERY] Hold mode !!\n\r");

	if (BMT_status.bat_vol < TALKING_RECHARGE_VOLTAGE || g_call_state == CALL_IDLE) {
		BMT_status.bat_charging_state = CHR_CC;
		battery_log(BAT_LOG_CRTI, "[BATTERY] Exit Hold mode and Enter CC mode !!\n\r");
	}

	/*  Disable charger */
	charging_enable = KAL_FALSE;
	battery_charging_control(CHARGING_CMD_ENABLE, &charging_enable);

	return PMU_STATUS_OK;
}


PMU_STATUS BAT_BatteryStatusFailAction(void)
{
	unsigned int charging_enable;

	battery_log(BAT_LOG_CRTI, "[BATTERY] BAD Battery status... Charging Stop !!\n\r");

#if defined(CONFIG_MTK_JEITA_STANDARD_SUPPORT)
	if ((g_temp_status == TEMP_ABOVE_POS_60) || (g_temp_status == TEMP_BELOW_NEG_10))
		temp_error_recovery_chr_flag = KAL_FALSE;

	if ((temp_error_recovery_chr_flag == KAL_FALSE) && (g_temp_status != TEMP_ABOVE_POS_60)
	    && (g_temp_status != TEMP_BELOW_NEG_10)) {
		temp_error_recovery_chr_flag = KAL_TRUE;
		BMT_status.bat_charging_state = CHR_PRE;
	}
#endif

	BMT_status.total_charging_time = 0;
	BMT_status.PRE_charging_time = 0;
	BMT_status.CC_charging_time = 0;
	BMT_status.TOPOFF_charging_time = 0;
	BMT_status.POSTFULL_charging_time = 0;

	/*  Disable charger */
	charging_enable = KAL_FALSE;
	battery_charging_control(CHARGING_CMD_ENABLE, &charging_enable);

	return PMU_STATUS_OK;
}


void mt_battery_charging_algorithm(void)
{
	battery_charging_control(CHARGING_CMD_RESET_WATCH_DOG_TIMER, NULL);

#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)
#if defined(PUMPEX_PLUS_RECHG)
	if (BMT_status.bat_in_recharging_state == KAL_TRUE && pep_det_rechg == KAL_TRUE)
		ta_check_chr_type = KAL_TRUE;
#endif
	battery_pump_express_charger_check();
#endif
	switch (BMT_status.bat_charging_state) {
	case CHR_PRE:
		BAT_PreChargeModeAction();
		break;

	case CHR_CC:
		BAT_ConstantCurrentModeAction();
		break;

	case CHR_BATFULL:
		BAT_BatteryFullAction();
		break;

	case CHR_HOLD:
		BAT_BatteryHoldAction();
		break;

	case CHR_ERROR:
		BAT_BatteryStatusFailAction();
		break;
	}

	battery_charging_control(CHARGING_CMD_DUMP_REGISTER, NULL);
}
