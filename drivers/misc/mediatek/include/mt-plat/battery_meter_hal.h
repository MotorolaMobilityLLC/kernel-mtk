#ifndef _BATTERY_METER_HAL_H
#define _BATTERY_METER_HAL_H


/* ============================================================ */
/* define */
/* ============================================================ */
#define BM_LOG_CRTI 1
#define BM_LOG_FULL 2

#define bm_print(num, fmt, args...)   \
do {									\
	if (Enable_FGADC_LOG >= (int)num) {				\
		pr_notice(fmt, ##args); \
	}								   \
} while (0)


/* ============================================================ */
/* ENUM */
/* ============================================================ */
typedef enum {
	BATTERY_METER_CMD_HW_FG_INIT,

	BATTERY_METER_CMD_GET_HW_FG_CURRENT,	/* fgauge_read_current */
	BATTERY_METER_CMD_GET_HW_FG_CURRENT_SIGN,	/*  */
	BATTERY_METER_CMD_GET_HW_FG_CAR,	/* fgauge_read_columb */

	BATTERY_METER_CMD_HW_RESET,	/* FGADC_Reset_SW_Parameter */

	BATTERY_METER_CMD_GET_ADC_V_BAT_SENSE,
	BATTERY_METER_CMD_GET_ADC_V_I_SENSE,
	BATTERY_METER_CMD_GET_ADC_V_BAT_TEMP,
	BATTERY_METER_CMD_GET_ADC_V_CHARGER,

	BATTERY_METER_CMD_GET_HW_OCV,
	BATTERY_METER_CMD_DUMP_REGISTER,
	BATTERY_METER_CMD_SET_COLUMB_INTERRUPT,
	BATTERY_METER_CMD_GET_BATTERY_PLUG_STATUS,
	BATTERY_METER_CMD_GET_HW_FG_CAR_ACT,	/* fgauge_read_columb */

	BATTERY_METER_CMD_NUMBER
} BATTERY_METER_CTRL_CMD;

#ifndef BATTERY_BOOL
#define BATTERY_BOOL
typedef enum {
	KAL_FALSE = 0,
	KAL_TRUE  = 1,
} kal_bool;
#endif


/* ============================================================ */
/* structure */
/* ============================================================ */

/* ============================================================ */
/* typedef */
/* ============================================================ */
typedef signed int(*BATTERY_METER_CONTROL) (BATTERY_METER_CTRL_CMD cmd, void *data);

/* ============================================================ */
/* External Variables */
/* ============================================================ */
extern int Enable_FGADC_LOG;

/* ============================================================ */
/* External function */
/* ============================================================ */
extern signed int bm_ctrl_cmd(BATTERY_METER_CTRL_CMD cmd, void *data);


#endif				/* #ifndef _BATTERY_METER_HAL_H */
