/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __LINUX_PROP_CHGALGO_CLASS_H
#define __LINUX_PROP_CHGALGO_CLASS_H

#include <linux/device.h>
#include <linux/notifier.h>

#define PCA_DBG(fmt, ...) pr_debug("[PCA]%s " fmt, __func__, ##__VA_ARGS__)
#define PCA_INFO(fmt, ...) pr_info("[PCA]%s " fmt, __func__, ##__VA_ARGS__)
#define PCA_ERR(fmt, ...) pr_info("[PCA]%s " fmt, __func__, ##__VA_ARGS__)

struct prop_chgalgo_ta_status {
	int temp1;
	int temp2;
	u8 temp_level;
	u8 present_input;
	u8 present_battery_input;
	bool ocp;
	bool otp;
	bool ovp;
	bool cc_mode;
};

struct prop_chgalgo_ta_auth_data {
	int cap_vmin;
	int cap_vmax;
	int cap_imin;
	int ta_vmin;
	int ta_vmax;
	int ta_imax;
};

enum prop_chgalgo_adc_channel {
	PCA_ADCCHAN_VBUS = 0,
	PCA_ADCCHAN_IBUS,
	PCA_ADCCHAN_VBAT,
	PCA_ADCCHAN_BIFVBAT,
	PCA_ADCCHAN_IBAT,
	PCA_ADCCHAN_TBAT,
	PCA_ADCCHAN_BIFTBAT,
	PCA_ADCCHAN_TCHG,
	PCA_ADCCHAN_VOUT,
	PCA_ADCCHAN_MAX,
};

struct prop_chgalgo_device;

struct prop_chgalgo_ta_ops {
	int (*enable_charging)(struct prop_chgalgo_device *pca, bool en, u32 mV,
			       u32 mA);
	int (*set_cap)(struct prop_chgalgo_device *pca, u32 mV, u32 mA);
	int (*get_measure_cap)(struct prop_chgalgo_device *pca, u32 *mV,
			       u32 *mA);
	int (*get_temperature)(struct prop_chgalgo_device *pca, int *degree);
	int (*get_status)(struct prop_chgalgo_device *pca,
			  struct prop_chgalgo_ta_status *status);
	int (*send_hardreset)(struct prop_chgalgo_device *pca);
	int (*authenticate_ta)(struct prop_chgalgo_device *pca,
			       struct prop_chgalgo_ta_auth_data *data);
	int (*enable_wdt)(struct prop_chgalgo_device *pca, bool en);
	int (*set_wdt)(struct prop_chgalgo_device *pca, u32 ms);
};

struct prop_chgalgo_chg_ops {
	int (*enable_power_path)(struct prop_chgalgo_device *pca, bool en);
	int (*enable_charging)(struct prop_chgalgo_device *pca, bool en);
	int (*enable_chip)(struct prop_chgalgo_device *pca, bool en);
	int (*enable_hz)(struct prop_chgalgo_device *pca, bool en);
	int (*set_vbusovp)(struct prop_chgalgo_device *pca, u32 mV);
	int (*set_ibusocp)(struct prop_chgalgo_device *pca, u32 mA);
	int (*set_vbatovp)(struct prop_chgalgo_device *pca, u32 mV);
	int (*set_ibatocp)(struct prop_chgalgo_device *pca, u32 mA);
	int (*set_aicr)(struct prop_chgalgo_device *pca, u32 mA);
	int (*set_ichg)(struct prop_chgalgo_device *pca, u32 mA);
	int (*get_adc)(struct prop_chgalgo_device *pca,
		       enum prop_chgalgo_adc_channel chan, int *min, int *max);
	bool (*is_bif_exist)(struct prop_chgalgo_device *pca);
	int (*get_soc)(struct prop_chgalgo_device *pca, u32 *soc);
};

struct prop_chgalgo_algo_ops {
	int (*init_algo)(struct prop_chgalgo_device *pca);
	bool (*is_algo_ready)(struct prop_chgalgo_device *pca);
	int (*start_algo)(struct prop_chgalgo_device *pca);
	bool (*is_algo_running)(struct prop_chgalgo_device *pca);
	int (*plugout_reset)(struct prop_chgalgo_device *pca);
	int (*stop_algo)(struct prop_chgalgo_device *pca);
};

enum prop_chgalgo_dev_type {
	PCA_DEVTYPE_TA = 0,
	PCA_DEVTYPE_CHARGER,
	PCA_DEVTYPE_ALGO,
	PCA_DEVTYPE_MAX,
};

struct prop_chgalgo_desc {
	const char *name;
	enum prop_chgalgo_dev_type type;
};

struct prop_chgalgo_device {
	struct device dev;
	struct prop_chgalgo_ta_ops *ta_ops;
	struct prop_chgalgo_chg_ops *chg_ops;
	struct prop_chgalgo_algo_ops *algo_ops;
	struct prop_chgalgo_desc *desc;
	void *drv_data;
};

extern struct prop_chgalgo_device *
prop_chgalgo_device_register(struct device *parent,
			     struct prop_chgalgo_desc *prop_chgalgo_desc,
			     struct prop_chgalgo_ta_ops *ta_ops,
			     struct prop_chgalgo_chg_ops *chg_ops,
			     struct prop_chgalgo_algo_ops *algo_ops,
			     void *drv_data);
extern void prop_chgalgo_device_unregister(struct prop_chgalgo_device *pca);
extern struct prop_chgalgo_device *
prop_chgalgo_dev_get_by_name(const char *name);

static inline int prop_chgalgo_get_devtype(struct prop_chgalgo_device *pca)
{
	return pca->desc->type;
}

static inline void *prop_chgalgo_get_drvdata(struct prop_chgalgo_device *pca)
{
	return pca->drv_data;
}

/* Richtek pca TA interface */
extern int prop_chgalgo_enable_ta_charging(struct prop_chgalgo_device *pca,
					   bool en, u32 mV, u32 mA);
extern int prop_chgalgo_set_ta_cap(struct prop_chgalgo_device *pca, u32 mV,
				   u32 mA);
extern int prop_chgalgo_get_ta_measure_cap(struct prop_chgalgo_device *pca,
					   u32 *mV, u32 *mA);
extern int prop_chgalgo_get_ta_temperature(struct prop_chgalgo_device *pca,
					   int *degree);
extern int prop_chgalgo_get_ta_status(struct prop_chgalgo_device *pca,
				      struct prop_chgalgo_ta_status *status);
extern int prop_chgalgo_send_ta_hardreset(struct prop_chgalgo_device *pca);
extern int prop_chgalgo_authenticate_ta(struct prop_chgalgo_device *pca,
					struct prop_chgalgo_ta_auth_data *data);
extern int prop_chgalgo_enable_ta_wdt(struct prop_chgalgo_device *pca, bool en);
extern int prop_chgalgo_set_ta_wdt(struct prop_chgalgo_device *pca, u32 ms);

/* Richtek pca charger interface */
extern int prop_chgalgo_enable_power_path(struct prop_chgalgo_device *pca,
					  bool en);
extern int prop_chgalgo_enable_charging(struct prop_chgalgo_device *pca,
					bool en);
extern int prop_chgalgo_enable_chip(struct prop_chgalgo_device *pca, bool en);
extern int prop_chgalgo_enable_hz(struct prop_chgalgo_device *pca, bool en);
extern int prop_chgalgo_set_vbusovp(struct prop_chgalgo_device *pca, u32 uV);
extern int prop_chgalgo_set_ibusocp(struct prop_chgalgo_device *pca, u32 uA);
extern int prop_chgalgo_set_vbatovp(struct prop_chgalgo_device *pca, u32 uV);
extern int prop_chgalgo_set_ibatocp(struct prop_chgalgo_device *pca, u32 uA);
extern int prop_chgalgo_get_adc(struct prop_chgalgo_device *pca,
				enum prop_chgalgo_adc_channel chan, int *min,
				int *max);
extern bool prop_chgalgo_is_bif_exist(struct prop_chgalgo_device *pca);
extern int prop_chgalgo_get_soc(struct prop_chgalgo_device *pca, u32 *soc);
extern int prop_chgalgo_set_ichg(struct prop_chgalgo_device *pca, u32 mA);
extern int prop_chgalgo_set_aicr(struct prop_chgalgo_device *pca, u32 mA);

/* Richtek pca algorithm interface */
#ifdef CONFIG_RT_PROP_CHGALGO
extern int prop_chgalgo_init_algo(struct prop_chgalgo_device *pca);
extern bool prop_chgalgo_is_algo_ready(struct prop_chgalgo_device *pca);
extern int prop_chgalgo_start_algo(struct prop_chgalgo_device *pca);
extern bool prop_chgalgo_is_algo_running(struct prop_chgalgo_device *pca);
extern int prop_chgalgo_plugout_reset(struct prop_chgalgo_device *pca);
extern int prop_chgalgo_stop_algo(struct prop_chgalgo_device *pca);
#else
static inline int prop_chgalgo_init_algo(struct prop_chgalgo_device *pca)
{
	return -ENOTSUPP;
}

static inline bool prop_chgalgo_is_algo_ready(struct prop_chgalgo_device *pca)
{
	return false;
}

static inline int prop_chgalgo_start_algo(struct prop_chgalgo_device *pca)
{
	return -ENOTSUPP;
}

static inline bool prop_chgalgo_is_algo_running(struct prop_chgalgo_device *pca)
{
	return false;
}

static inline int prop_chgalgo_plugout_reset(struct prop_chgalgo_device *pca)
{
	return -ENOTSUPP;
}

static inline int prop_chgalgo_stop_algo(struct prop_chgalgo_device *pca)
{
	return -ENOTSUPP;
}
#endif /* CONFIG_RT_PROP_CHGALGO */
#endif /* __LINUX_PROP_CHGALGO_CLASS_H */
