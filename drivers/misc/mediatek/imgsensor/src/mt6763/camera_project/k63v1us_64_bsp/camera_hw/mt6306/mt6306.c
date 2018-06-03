/*
 * Copyright (C) 2017 MediaTek Inc.
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

#include "mt6306.h"

const unsigned long mt6306_pin_list[MT6306_PIN_MAX_NUM] = {
	MT6306_GPIO_05,
	MT6306_GPIO_10,
	MT6306_GPIO_02,
	MT6306_GPIO_03,
	MT6306_GPIO_08, /* PDN2 */
	MT6306_GPIO_07, /* RST2 */
	MT6306_GPIO_02, /* Main2 VCAMD*/
	MT6306_GPIO_04, /* MIPI_SWITCH_EN */
	MT6306_GPIO_05  /* MIPI_SWITCH_SEL */
};


static struct MT6306 mt6306_instance;

static enum IMGSENSOR_RETURN mt6306_init(void *instance)
{	int i = 0;

	for (i = 0; i < MT6306_PIN_MAX_NUM; i++)
		atomic_set(&mt6306_instance.enable_cnt[i], 0);
	return IMGSENSOR_RETURN_SUCCESS;
}

static enum IMGSENSOR_RETURN mt6306_release(void *instance)
{
	int i = 0;

	for (i = 0; i < MT6306_PIN_MAX_NUM; i++) {
		atomic_set(&mt6306_instance.enable_cnt[i], 0);
		mt6306_set_gpio_out(mt6306_pin_list[i], MT6306_GPIO_OUT_LOW);
	}
	return IMGSENSOR_RETURN_SUCCESS;
}

static enum IMGSENSOR_RETURN mt6306_set(
	void *pinstance,
	enum IMGSENSOR_SENSOR_IDX   sensor_idx,
	enum IMGSENSOR_HW_PIN       pin,
	enum IMGSENSOR_HW_PIN_STATE pin_state)
{
	int pin_offset;
	int list_idx = 0;
	const unsigned long *ppin_list;
	enum IMGSENSOR_RETURN ret = IMGSENSOR_RETURN_SUCCESS;

#ifdef MIPI_SWITCH
	if (pin < IMGSENSOR_HW_PIN_PDN ||
	   pin > IMGSENSOR_HW_PIN_MIPI_SWITCH_SEL ||
	   pin_state < IMGSENSOR_HW_PIN_STATE_LEVEL_0 ||
	   pin_state > IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH)
		ret = IMGSENSOR_RETURN_ERROR;
#else
	if (pin < IMGSENSOR_HW_PIN_PDN ||
	   pin > IMGSENSOR_HW_PIN_DVDD ||
	   pin_state < IMGSENSOR_HW_PIN_STATE_LEVEL_0 ||
	   pin_state > IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH)
		ret = IMGSENSOR_RETURN_ERROR;
#endif

	if (pin == IMGSENSOR_HW_PIN_AVDD ||
	  (pin == IMGSENSOR_HW_PIN_DVDD &&
	   sensor_idx == IMGSENSOR_SENSOR_IDX_MAIN2) ||
	    (pin == IMGSENSOR_HW_PIN_DVDD &&
	   sensor_idx == IMGSENSOR_SENSOR_IDX_SUB2)) {
		list_idx = MT6306_PIN_CAM_EXT_PWR_EN;
		PK_DBG("mt6306_set pin number %d value %d\n", MT6306_PIN_CAM_EXT_PWR_EN, pin_state);
	} else if (pin == IMGSENSOR_HW_PIN_MIPI_SWITCH_EN) {
		list_idx = MT6306_PIN_CAM_MIPI_SWITCH_EN;
		PK_DBG("mt6306_set MIPI_SWITCH_EN pin number %d value %d\n", MT6306_PIN_CAM_MIPI_SWITCH_EN, pin_state);
	} else if (pin == IMGSENSOR_HW_PIN_MIPI_SWITCH_SEL) {
		list_idx = MT6306_PIN_CAM_MIPI_SWITCH_SEL;
		PK_DBG("mt6306_set MIPI_SWITCH_SEL pin number %d value %d\n",
			 MT6306_PIN_CAM_MIPI_SWITCH_SEL, pin_state);
	} else {
		pin_offset = (sensor_idx == IMGSENSOR_SENSOR_IDX_MAIN) ? MT6306_PIN_CAM_PDN0 :
					 (sensor_idx == IMGSENSOR_SENSOR_IDX_SUB)  ? MT6306_PIN_CAM_PDN1 :
					 MT6306_PIN_CAM_PDN2;

		list_idx = pin_offset + pin - IMGSENSOR_HW_PIN_PDN;
		PK_DBG("mt6306_set pin number %d value %d\n", pin_offset + pin - IMGSENSOR_HW_PIN_PDN, pin_state);
	}
	PK_DBG("mt6306_set pin number %d value %d\n", list_idx, pin_state);
	ppin_list  = &mt6306_pin_list[list_idx];

	mt6306_set_gpio_dir(*ppin_list, MT6306_GPIO_DIR_OUT);

	if (pin_state > IMGSENSOR_HW_PIN_STATE_LEVEL_0) {
		if (atomic_read(&mt6306_instance.enable_cnt[list_idx]) == 0)
			mt6306_set_gpio_out(*ppin_list, MT6306_GPIO_OUT_HIGH);

		atomic_inc(&mt6306_instance.enable_cnt[list_idx]);
	} else {
		if (atomic_read(&mt6306_instance.enable_cnt[list_idx]) == 1)
			mt6306_set_gpio_out(*ppin_list, MT6306_GPIO_OUT_LOW);

		if (atomic_read(&mt6306_instance.enable_cnt[list_idx]) > 0)
			atomic_dec(&mt6306_instance.enable_cnt[list_idx]);
	}

	return ret;
}

static struct IMGSENSOR_HW_DEVICE device = {
	.pinstance = (void *)&mt6306_instance,
	.init      = mt6306_init,
	.set       = mt6306_set,
	.release   = mt6306_release,
	.id        = IMGSENSOR_HW_ID_MT6306
};

enum IMGSENSOR_RETURN imgsensor_hw_mt6306_open(
	struct IMGSENSOR_HW_DEVICE **pdevice)
{
	*pdevice = &device;
	return IMGSENSOR_RETURN_SUCCESS;
}

