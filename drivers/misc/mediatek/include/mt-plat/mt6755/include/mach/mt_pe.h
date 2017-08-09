#ifndef _CUST_PE_H_
#define _CUST_PE_H_

/* Pump Express support (fast charging) */
#ifdef CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT
#define TA_START_BATTERY_SOC	1
#define TA_STOP_BATTERY_SOC	95
#define TA_AC_12V_INPUT_CURRENT CHARGE_CURRENT_3200_00_MA
#define TA_AC_9V_INPUT_CURRENT	CHARGE_CURRENT_3200_00_MA
#define TA_AC_7V_INPUT_CURRENT	CHARGE_CURRENT_3200_00_MA
#define TA_AC_CHARGING_CURRENT	CHARGE_CURRENT_3000_00_MA
#define TA_9V_SUPPORT
#define TA_12V_SUPPORT

#define SWITCH_CHR_VINDPM_5V 0x12  /* 4.4V */
#define SWITCH_CHR_VINDPM_7V 0x25  /* 6.3V */
#define SWITCH_CHR_VINDPM_9V 0x37  /* 8.1V */
#define SWITCH_CHR_VINDPM_12V 0x54 /* 11.0 set this tp prevent adapters from failure and reset*/

/*this is for HW OVP*/
#define TA_AC_12V_INPUT_OVER_VOLTAGE BATTERY_VOLT_10_500000_V
#define TA_AC_9V_INPUT_OVER_VOLTAGE  BATTERY_VOLT_10_500000_V
#define TA_AC_7V_INPUT_OVER_VOLTAGE  BATTERY_VOLT_09_000000_V
#define TA_AC_5V_INPUT_OVER_VOLTAGE  BATTERY_VOLT_07_000000_V

/*Pump express plus : recharge enable mode*/
#define PUMPEX_PLUS_RECHG (1)

/*This seems not to be used batterymeter*/
#undef V_CHARGER_MAX
#if defined(TA_9V_SUPPORT) || defined(TA_12V_SUPPORT)
#define V_CHARGER_MAX 13500				/* For SW OVP*/
#else
#define V_CHARGER_MAX 7500				/* 7.5 V */
#endif
#endif

#endif /* _CUST_PE_H_ */
