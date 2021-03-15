/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __SITUATION_H__
#define __SITUATION_H__

#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <hwmsensor.h>
#include <linux/poll.h>
#include "sensor_attr.h"
#include "sensor_event.h"
#include <linux/pm_wakeup.h>

enum situation_index_table {
	inpocket = 0,
	stationary,
	wake_gesture,
	pickup_gesture,
	glance_gesture,
	answer_call,
	motion_detect,
	device_orientation,
	tilt_detector,
	flat,
	sar,
/*new add for moto sensor algo*/
	stowed,
	flatup,
	flatdown,
	camgest,
	chopchop,
	mot_glance,
	ltv,
	ftm,
	offbody,
	lts,
	max_situation_support,
};

struct situation_control_path {
	int (*open_report_data)(int open);
	int (*batch)(int flag, int64_t samplingPeriodNs,
		int64_t maxBatchReportLatencyNs);
	int (*flush)(void);
	bool is_support_wake_lock;
	bool is_support_batch;
};

struct situation_data_path {
	int (*get_data)(int *value, int *status);
};

struct situation_init_info {
	char *name;
	int (*init)(void);
	int (*uninit)(void);
};

struct situation_data_control_context {
	struct situation_control_path situation_ctl;
	struct situation_data_path situation_data;
	bool is_active_data;
	bool is_active_nodata;
	bool is_batch_enable;
	int power;
	int enable;
	int64_t delay_ns;
	int64_t latency_ns;
};

/*moto add for sensor algo params*/
#ifdef CONFIG_MOTO_ALGO_PARAMS
struct mot_chopchop {
    float max_gyro_rotation;
    uint32_t max_chop_duration_ms;
    float first_accel_threshold;
    float second_accel_threshold;
    float min_magnitude_percentage;
    float max_xy_percentage;
    bool gyroless;
};
struct mot_glance {
    float motion_threshold;
    uint64_t cool_time;
    uint16_t gesture_allow;
    uint64_t uncover_timeout;
};
struct mot_ltv {
    float min_accel_rotation;
    float accel_fast_alpha;
    float accel_slow_alpha;
    uint8_t land_conv_cnt;
    float min_accel_rotation_wake;
    float non_accel_rotation_ff;
    float rearm_non_rotation;
    float rearm_min_accel_rotation;
    float rearm_forced_accel_rotation;
    uint8_t rearm_conv_cnt;
    float raw_accel_margin;
};
struct mot_offbody {
    float alpha;
    float var_lt;
    float var_ht;
    uint16_t th_cnt;
    float st_alpha;
    float st_var_ht;
    float theta_exit;
    float theta_low;
    float theta_high;
    bool qual_mode;
};

#ifdef CONFIG_MOTO_ALSPS
struct MotAlspsDefConfig {
  uint16_t pgain;
  uint16_t ptime;
  uint16_t pwtime;
  uint16_t ppulse;
  uint16_t pldrive;
  uint16_t ppulse_len;
  uint16_t ppers;
  uint16_t pBinSearch;
  float pOffsetStep;
  float pOffsetStepCoef;
  float low_coef;
  float high_coef;
  uint16_t farTh;
  uint16_t NearTh;
  uint16_t min_delta;
  bool pUseOilAlgo;
};

struct MotAlspsVariableConfig {
  uint16_t farTh;
  uint16_t NearTh;
  uint16_t pWTime;
  bool pUseOilAlgo;
};

struct MotAlspsPsAlgoConfig {
  uint16_t veryNearTh;
  uint16_t veryNearCnt;
  uint16_t contaminatedTh;
  uint16_t contaminatedCnt;
  uint16_t jitterTh;
};

struct MotAlspsAlsCoefficient {
  float v[12];
};

struct MotAlspsFacGoldenValue {
  uint16_t near_threshold;
  uint16_t far_threshold;
  float lux_scale;
};

struct MotAlsspsParams {
      struct MotAlspsDefConfig def_config;
      struct MotAlspsVariableConfig variable_config;
      struct MotAlspsPsAlgoConfig ps_algo_config;
      struct MotAlspsAlsCoefficient als_coefficient;
      struct MotAlspsFacGoldenValue fac_golden_value;
};

struct MotAlspsCfgData {
    int32_t cfg_type;
    union {
        struct MotAlsspsParams params;
        int32_t caliCfgData[8];
    };
};
#endif

struct mot_params {
#ifdef CONFIG_MOTO_CHOPCHOP
	struct mot_chopchop chopchop_params;
#endif
#ifdef CONFIG_MOTO_GLANCE
    struct mot_glance glance_params;
#endif
#ifdef CONFIG_MOTO_LTV
    struct mot_ltv ltv_params;
#endif
#ifdef CONFIG_MOTO_OFFBODY
    struct mot_offbody offbody_params;
#endif
#ifdef CONFIG_MOTO_ALSPS
    struct MotAlspsCfgData alsps_params;
#endif
};
#endif

struct situation_context {
	struct sensor_attr_t mdev;
	struct mutex situation_op_mutex;
	struct situation_data_control_context
		ctl_context[max_situation_support];
	struct wakeup_source *ws[max_situation_support];
	char *wake_lock_name[max_situation_support];
//moto add for sensor algo transfer params to scp
#ifdef CONFIG_MOTO_ALGO_PARAMS
	atomic_t first_ready_after_boot;
	struct work_struct init_done_work;
	atomic_t scp_init_done;
	struct mot_params motparams;
#endif
};

extern int situation_data_report_t(int handle, uint32_t one_sample_data,
	int64_t time_stamp);
extern int situation_data_report(int handle, uint32_t one_sample_data);
extern int situation_notify_t(int handle, int64_t time_stamp);
extern int situation_notify(int handle);
extern int situation_flush_report(int handle);
extern int situation_driver_add(struct situation_init_info *obj, int handle);
extern int situation_register_control_path(
	struct situation_control_path *ctl, int handle);
extern int situation_register_data_path(struct situation_data_path *data,
	int handle);
extern int sar_data_report(int32_t value[3]);
extern int sar_data_report_t(int32_t value[3], int64_t time_stamp);
extern int mot_ltv_data_report(int32_t value[3]);
#endif
