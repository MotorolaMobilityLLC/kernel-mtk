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

#include "mclk.h"

static struct mclk mclk_instance;

static enum IMGSENSOR_RETURN mclk_init(void *pinstance)
{
	struct mclk *pinst = (struct mclk *)pinstance;

	pinst->pmclk_func[MCLK_MODULE_1] = ISP_MCLK1_EN;
	pinst->pmclk_func[MCLK_MODULE_2] = ISP_MCLK2_EN;
	pinst->pmclk_func[MCLK_MODULE_3] = ISP_MCLK3_EN;
	pinst->pmclk_func[MCLK_MODULE_4] = ISP_MCLK4_EN;

	return IMGSENSOR_RETURN_SUCCESS;
}

static enum IMGSENSOR_RETURN mclk_set(
	void *pinstance,
	enum IMGSENSOR_SENSOR_IDX   sensor_idx,
	enum IMGSENSOR_HW_PIN       pin,
	enum IMGSENSOR_HW_PIN_STATE pin_state)
{
	struct mclk *pinst = (struct mclk *)pinstance;

	if(pin < IMGSENSOR_HW_PIN_MCLK1 ||
	   pin > IMGSENSOR_HW_PIN_MCLK4 ||
	   pin_state < IMGSENSOR_HW_PIN_STATE_LEVEL_0 ||
	   pin_state > IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH) {
		return IMGSENSOR_RETURN_ERROR;
	} else {
		pin_state = (pin_state > IMGSENSOR_HW_PIN_STATE_LEVEL_0) ? MCLK_STATE_ENABLE : MCLK_STATE_DISABLE;
		pinst->pmclk_func[pin - IMGSENSOR_HW_PIN_MCLK1] ((BOOL)pin_state);
		return IMGSENSOR_RETURN_SUCCESS;
	}
}

static struct IMGSENSOR_HW_DEVICE device = {
	.pinstance = (void *)&mclk_instance,
	.init      = mclk_init,
	.set       = mclk_set,
	.id        = IMGSENSOR_HW_ID_MCLK
};

enum IMGSENSOR_RETURN imgsensor_hw_mclk_open(
	struct IMGSENSOR_HW_DEVICE **pdevice)
{
	*pdevice = &device;
	return IMGSENSOR_RETURN_SUCCESS;
}

