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
	MT6306_GPIO_11,
	MT6306_GPIO_04,
	MT6306_GPIO_12
};


static MT6306 mt6306_instance;

static enum IMGSENSOR_RETURN mt6306_init(void *instance)
{
	return IMGSENSOR_RETURN_SUCCESS;
}

static enum IMGSENSOR_RETURN mt6306_set(
	void *pinstance,
	enum IMGSENSOR_SENSOR_IDX   sensor_idx,
	enum IMGSENSOR_HW_PIN       pin,
	enum IMGSENSOR_HW_PIN_STATE pin_state)
{
	int pin_offset;
	const unsigned long *ppin_list;

	if(pin < IMGSENSOR_HW_PIN_PDN ||
	   pin > IMGSENSOR_HW_PIN_DVDD||
	   pin_state < IMGSENSOR_HW_PIN_STATE_LEVEL_0 ||
	   pin_state > IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH)
		return IMGSENSOR_RETURN_ERROR;

	if(pin == IMGSENSOR_HW_PIN_AVDD ||
	  (pin == IMGSENSOR_HW_PIN_DVDD &&
	   sensor_idx == IMGSENSOR_SENSOR_IDX_MAIN2)) {
		ppin_list  = &mt6306_pin_list[MT6306_PIN_CAM_EXT_PWR_EN];

	} else {
		pin_offset = (sensor_idx == IMGSENSOR_SENSOR_IDX_MAIN) ? MT6306_PIN_CAM_PDN0 :
					 (sensor_idx == IMGSENSOR_SENSOR_IDX_SUB)  ? MT6306_PIN_CAM_PDN1 :
																 MT6306_PIN_CAM_PDN2;
		ppin_list  = &mt6306_pin_list[pin_offset + pin - IMGSENSOR_HW_PIN_PDN];
	}

	mt6306_set_gpio_dir(*ppin_list, MT6306_GPIO_DIR_OUT);
	mt6306_set_gpio_out(*ppin_list, (pin_state > IMGSENSOR_HW_PIN_STATE_LEVEL_0) ?
		MT6306_GPIO_OUT_HIGH : MT6306_GPIO_OUT_LOW);

	return IMGSENSOR_RETURN_SUCCESS;
}

static struct IMGSENSOR_HW_DEVICE device = {
	.pinstance = (void *)&mt6306_instance,
	.init      = mt6306_init,
	.set       = mt6306_set,
	.id        = IMGSENSOR_HW_ID_MT6306
};

enum IMGSENSOR_RETURN imgsensor_hw_mt6306_open(
	struct IMGSENSOR_HW_DEVICE **pdevice)
{
	*pdevice = &device;
	return IMGSENSOR_RETURN_SUCCESS;
}

