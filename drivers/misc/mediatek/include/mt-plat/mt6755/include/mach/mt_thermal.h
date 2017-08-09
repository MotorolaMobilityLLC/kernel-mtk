#ifndef _MT6755_THERMAL_H
#define _MT6755_THERMAL_H

#include <linux/module.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include "mt-plat/sync_write.h"
#include "mtk_thermal_typedefs.h"
#include "mt_gpufreq.h"

/* 1: thermal catm turn on:1, turn off:0*/
#define THERMAL_CATM_USER  (0)
/*
Bank0 : CPU_mp0     (TS_MCU4)
Bank1 : SOC+GPU     (TS_MCU1, TS_MCU2, TS_MCU3)
Bank2 : CPU_mp1     (TS_MCU4)
*/
typedef enum {
	THERMAL_SENSOR1 = 0,	/*TS_MCU1 */
	THERMAL_SENSOR2 = 1,	/*TS_MCU2 */
	THERMAL_SENSOR3 = 2,	/*TS_MCU3 */
	THERMAL_SENSOR4 = 3,	/*TS_MCU4 */
	THERMAL_SENSOR_ABB = 4,	/*TS_ABB */
	THERMAL_SENSOR_NUM
} thermal_sensor_name;

typedef enum {
	THERMAL_BANK0 = 0,	/* CPU           (TSMCU2) */
	THERMAL_BANK1 = 1,	/* SOC + GPU (TSMCU1) */
	THERMAL_BANK2 = 2,	/* LTE       (TSMCU3) */
	THERMAL_BANK_NUM
} thermal_bank_name;


struct TS_PTPOD {
	unsigned int ts_MTS;
	unsigned int ts_BTS;
};

extern int mtktscpu_limited_dmips;

extern void get_thermal_slope_intercept(struct TS_PTPOD *ts_info, thermal_bank_name ts_bank);
extern void set_taklking_flag(bool flag);
extern int tscpu_get_cpu_temp(void);
extern int tscpu_get_temp_by_bank(thermal_bank_name ts_bank);

#define THERMAL_WRAP_WR32(val, addr)        mt_reg_sync_writel((val), ((void *)addr))


extern int get_immediate_cpu0_wrap(void);
extern int get_immediate_gpu_wrap(void);
extern int get_immediate_cpu1_wrap(void);

/*add for DLPT*/
extern int tscpu_get_min_cpu_pwr(void);
extern int tscpu_get_min_gpu_pwr(void);

/*4 thermal sensors*/
typedef enum {
	MTK_THERMAL_SENSOR_TS1 = 0,
	MTK_THERMAL_SENSOR_TS2,
	MTK_THERMAL_SENSOR_TS3,
	MTK_THERMAL_SENSOR_TS4,
	MTK_THERMAL_SENSOR_TSABB,

	ATM_CPU_LIMIT,
	ATM_GPU_LIMIT,

	MTK_THERMAL_SENSOR_CPU_COUNT
} MTK_THERMAL_SENSOR_CPU_ID_MET;

typedef enum {
	TA_DAEMON_CMD_GET_INIT_FLAG = 0,
	TA_DAEMON_CMD_SET_DAEMON_PID,
	TA_DAEMON_CMD_NOTIFY_DAEMON,
	TA_DAEMON_CMD_NOTIFY_DAEMON_CATMINIT,
	TA_DAEMON_CMD_SET_TTJ,
	TA_DAEMON_CMD_GET_TPCB,

	TA_DAEMON_CMD_TO_KERNEL_NUMBER
} TA_DAEMON_CTRL_CMD_TO_KERNEL; /*must sync userspace/kernel: TA_DAEMON_CTRL_CMD_FROM_USER*/

#define TAD_NL_MSG_T_HDR_LEN 12
#define TAD_NL_MSG_MAX_LEN 2048

struct tad_nl_msg_t {
	unsigned int tad_cmd;
	unsigned int tad_data_len;
	unsigned int tad_ret_data_len;
	char tad_data[TAD_NL_MSG_MAX_LEN];
};

enum {
	TA_CATMPLUS = 1,
	TA_CONTINUOUS = 2,
	TA_CATMPLUS_TTJ = 3
};


struct cATM_params_t {
	int CATM_ON;
	int K_TT;
	int K_SUM_TT_LOW;
	int K_SUM_TT_HIGH;
	int MIN_SUM_TT;
	int MAX_SUM_TT;
	int MIN_TTJ;
	int CATMP_STEADY_TTJ_DELTA;
};
struct continuetm_params_t {
	int STEADY_TARGET_TJ;
	int MAX_TARGET_TJ;
	int TRIP_TPCB;
	int STEADY_TARGET_TPCB;
};


struct CATM_T {
	struct cATM_params_t t_catm_par;
	struct continuetm_params_t t_continuetm_par;
};
extern struct CATM_T thermal_atm_t;
int wakeup_ta_algo(int flow_state);
int ta_get_ttj(void);


extern int tscpu_get_cpu_temp_met(MTK_THERMAL_SENSOR_CPU_ID_MET id);


typedef void (*met_thermalsampler_funcMET) (void);
extern void mt_thermalsampler_registerCB(met_thermalsampler_funcMET pCB);

extern void tscpu_start_thermal(void);
extern void tscpu_stop_thermal(void);
extern void tscpu_cancel_thermal_timer(void);
extern void tscpu_start_thermal_timer(void);
extern void mtkts_btsmdpa_cancel_thermal_timer(void);
extern void mtkts_btsmdpa_start_thermal_timer(void);
extern void mtkts_bts_cancel_thermal_timer(void);
extern void mtkts_bts_start_thermal_timer(void);
extern void mtkts_pmic_cancel_thermal_timer(void);
extern void mtkts_pmic_start_thermal_timer(void);
extern void mtkts_battery_cancel_thermal_timer(void);
extern void mtkts_battery_start_thermal_timer(void);
extern void mtkts_pa_cancel_thermal_timer(void);
extern void mtkts_pa_start_thermal_timer(void);
extern void mtkts_wmt_cancel_thermal_timer(void);
extern void mtkts_wmt_start_thermal_timer(void);

extern void mtkts_allts_cancel_ts1_timer(void);
extern void mtkts_allts_start_ts1_timer(void);
extern void mtkts_allts_cancel_ts2_timer(void);
extern void mtkts_allts_start_ts2_timer(void);
extern void mtkts_allts_cancel_ts3_timer(void);
extern void mtkts_allts_start_ts3_timer(void);
extern void mtkts_allts_cancel_ts4_timer(void);
extern void mtkts_allts_start_ts4_timer(void);
extern void mtkts_allts_cancel_ts5_timer(void);
extern void mtkts_allts_start_ts5_timer(void);

extern int mtkts_bts_get_hw_temp(void);

extern int get_immediate_ts1_wrap(void);
extern int get_immediate_ts2_wrap(void);
extern int get_immediate_ts3_wrap(void);
extern int get_immediate_ts4_wrap(void);
extern int get_immediate_tsabb_wrap(void);

/* Get TS_ temperatures from its thermal_zone instead of raw data,
 * temperature here would be processed according to the policy setting */
extern int mtkts_get_ts1_temp(void);
extern int mtkts_get_ts2_temp(void);
extern int mtkts_get_ts3_temp(void);
extern int mtkts_get_ts4_temp(void);
extern int mtkts_get_tsabb_temp(void);

extern int is_cpu_power_unlimit(void);	/* in mtk_ts_cpu.c */
extern int is_cpu_power_min(void);	/* in mtk_ts_cpu.c */
extern int get_cpu_target_tj(void);
extern int get_cpu_target_offset(void);
extern int mtk_gpufreq_register(struct mt_gpufreq_power_table_info *freqs, int num);

extern int get_target_tj(void);
extern int mtk_thermal_get_tpcb_target(void);

#endif
