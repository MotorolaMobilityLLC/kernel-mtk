/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2023. All rights reserved.
 * 2023-12-28 File created.
 */

#ifndef __FRSM_AMP_DRV_H__
#define __FRSM_AMP_DRV_H__

#include <linux/debugfs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include "frsm-dev.h"

#define FRSM_AMP_NAME "frsm_amp"

// FSADSP MODULE ID
#define AFE_MODULE_ID_FSADSP_TX (0x10001110)
#define AFE_MODULE_ID_FSADSP_RX (0x10001111)

// FSADSP PARAM ID
#define CAPI_V2_PARAM_FSADSP_TX_ENABLE     0x11111601
#define CAPI_V2_PARAM_FSADSP_RX_ENABLE     0x11111611
#define CAPI_V2_PARAM_FSADSP_MODULE_ENABLE 0x10001FA1
#define CAPI_V2_PARAM_FSADSP_MODULE_PARAM  0x10001FA2
#define CAPI_V2_PARAM_FSADSP_LIVEDATA      0x10001FA6
#define CAPI_V2_PARAM_FSADSP_RE25          0x10001FA7
#define CAPI_V2_PARAM_FSADSP_VER           0x10001FAA
#define CAPI_V2_PARAM_FSADSP_CALIB         0x10001FAB
#define CAPI_V2_PARAM_FSADSP_PARAM_ACDB    0x10001FB0
#define CAPI_V2_PARAM_FSADSP_SYSTEM_INFO   0x10001FC5
#define CAPI_V2_PARAM_FSADSP_DTYPE         0x10001FC8

#define FRSM_CALIB_PARAMS_V1  0xA001
#define FRSM_PAYLOAD_MAX      (64)

enum frsm_func {
	FRSM_HAS_MNTR = 0,
	FRSM_HAS_MISC,
	FRSM_HAS_CLASS,
	FRSM_HAS_TX_VI,
	FRSM_HAS_RX_SPC,
	FRSM_FUNC_MAX,
};

enum frsm_amp_event {
	EVENT_AMP_REG_READ = 0,
	EVENT_AMP_REG_WRITE,
	EVENT_AMP_SET_BATT,
	EVENT_AMP_SET_CALRE,
	EVENT_AMP_GET_LIVEDATA,
	EVENT_AMP_SPK_SWITCH,
	EVENT_AMP_SET_TUNING,
	EVENT_AMP_GET_MNTREN,
	EVENT_AMP_MUTE_SYNC,
	EVENT_AMP_INIT_DEV,
	EVENT_AMP_SET_SCENE,
	EVENT_AMP_GET_DTYPE,
	EVENT_AMP_MAX,
};

struct frsm_batt {
	int batv;
	int cap;
	int tempr;
};

struct frsm_amp_reg {
	char addr;
	int size;
	char buf[];
};

struct frsm_mode_params {
	char spkid;
	char mode;
};

struct frsm_adsp_pkg {
	int module_id;
	int param_id;
	int size;
	int buf[];
};

struct frsm_calib_params {
	uint16_t version;
	uint16_t ndev;
	struct __calib_info {
		uint16_t rstrim;
		uint16_t channel;
		int re25;
	} info[FRSM_DEV_MAX];
} __packed;

struct frsm_amp {
	struct device *dev;
	struct class class_dev;
	struct miscdevice misc_dev;
	struct workqueue_struct *thread_wq;
	struct delayed_work delay_work;
	struct delayed_work prepare_work;

	struct frsm_batt batt;
	struct spkr_info spkinfo;

	unsigned long state;
	unsigned long func;
	int mntr_period;
	int mntr_avg_count;

	unsigned int misc_cmd;
	char addr;
	char reg;

	bool prot_prepared;
	bool is_tuning;
	bool stream_on;
	bool calib_mode;
	bool mntr_en;
};

struct frsm_amp *frsm_amp_get_pdev(void);
int frsm_amp_notify(struct frsm_amp *frsm_amp, int event,
		struct frsm_argv *argv);
int frsm_amp_set_tuning(struct frsm_amp *frsm_amp,
		struct frsm_argv *argv);
int frsm_amp_set_calre(struct frsm_amp *frsm_amp,
		struct frsm_argv *argv);
int frsm_amp_get_livedata(struct frsm_amp *frsm_amp,
		struct frsm_argv *argv);
int frsm_amp_set_fsalgo(struct frsm_amp *frsm_amp,
		struct frsm_adsp_pkg *pkg);
int frsm_amp_mntr_switch(struct frsm_amp *frsm_amp, bool enable);
int frsm_amp_prot_battery(struct frsm_amp *frsm_amp, bool restore);

void frsm_amp_register_notify_callback(int (*func)(
		int event, void *buf, int size));
int frsm_amp_send_event(int event, void *buf, int size);
int frsm_amp_init_dev(int spkid, bool force);
int frsm_amp_set_scene(int spkid, int scene);
int frsm_amp_spk_switch(int spkid, bool on);

#endif // __FRSM_AMP_DRV_H__
