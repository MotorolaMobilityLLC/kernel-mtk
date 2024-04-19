/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __IMGSENSOR_HW_GPIO_H__
#define __IMGSENSOR_HW_GPIO_H__

#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinctrl.h>
#include "imgsensor_hw.h"
#include "imgsensor_common.h"
#include <imgsensor_platform.h>

enum GPIO_CTRL_STATE_CAM {
	/* Main */
	GPIO_CTRL_STATE_PDN_H,
	GPIO_CTRL_STATE_PDN_L,
	GPIO_CTRL_STATE_RST_H,
	GPIO_CTRL_STATE_RST_L,
	GPIO_CTRL_STATE_LDO_VCAMA_H,
	GPIO_CTRL_STATE_LDO_VCAMA_L,
	GPIO_CTRL_STATE_LDO_VCAMA1_H,
	GPIO_CTRL_STATE_LDO_VCAMA1_L,
	GPIO_CTRL_STATE_LDO_VCAMAFVDD_H,
	GPIO_CTRL_STATE_LDO_VCAMAFVDD_L,
#if defined(CONFIG_MOT_DEVONN_CAMERA_PROJECT) || defined(CONFIG_MOT_DEVONF_CAMERA_PROJECT) || defined(CONFIG_MOT_CANCUNF_CAMERA_PROJECT) || defined(CONFIG_MOT_CANCUNN_CAMERA_PROJECT) || defined(CONFIG_MOT_TAIPEI_CAMERA_PROJECT)
	GPIO_CTRL_STATE_LDO_VCAMAF_H,
	GPIO_CTRL_STATE_LDO_VCAMAF_L,
#endif
	GPIO_CTRL_STATE_LDO_VCAMD_H,
	GPIO_CTRL_STATE_LDO_VCAMD_L,
#if defined(CONFIG_MOT_CANCUNF_CAMERA_PROJECT) || defined(CONFIG_MOT_CANCUNN_CAMERA_PROJECT) || defined(CONFIG_MOT_TAIPEI_CAMERA_PROJECT)
	GPIO_CTRL_STATE_LDO_VCAMD1_H,
	GPIO_CTRL_STATE_LDO_VCAMD1_L,
#endif
	GPIO_CTRL_STATE_LDO_VCAMIO_H,
	GPIO_CTRL_STATE_LDO_VCAMIO_L,
	GPIO_CTRL_STATE_MAX_NUM_CAM,
};

/* for mipi switch platform */
enum GPIO_CTRL_STATE_SWITCH {
	GPIO_CTRL_STATE_MIPI_SWITCH_EN_H,
	GPIO_CTRL_STATE_MIPI_SWITCH_EN_L,
	GPIO_CTRL_STATE_MIPI_SWITCH_SEL_H,
	GPIO_CTRL_STATE_MIPI_SWITCH_SEL_L,
	GPIO_CTRL_STATE_MAX_NUM_SWITCH,
};


enum GPIO_STATE {
	GPIO_STATE_H,
	GPIO_STATE_L,
};

struct GPIO_PINCTRL {
	char *ppinctrl_lookup_names;
};

struct GPIO {
	struct pinctrl       *ppinctrl;
	struct pinctrl_state *ppinctrl_state_cam[
		IMGSENSOR_SENSOR_IDX_MAX_NUM][GPIO_CTRL_STATE_MAX_NUM_CAM];
	/* for mipi switch platform */
	struct pinctrl       *ppinctrl_switch;
	struct pinctrl_state *ppinctrl_state_switch[
		GPIO_CTRL_STATE_MAX_NUM_SWITCH];
	struct mutex         *pgpio_mutex;
};

enum IMGSENSOR_RETURN
imgsensor_hw_gpio_open(struct IMGSENSOR_HW_DEVICE **pdevice);

#endif

