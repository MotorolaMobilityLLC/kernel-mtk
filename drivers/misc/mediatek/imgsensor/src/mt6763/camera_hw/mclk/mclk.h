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

#ifndef __IMGSENSOR_HW_MCLK_h__
#define __IMGSENSOR_HW_MCLK_h__

#include "kd_camera_typedef.h"
#include "imgsensor_hw.h"
#include "imgsensor_common.h"

enum MCLK_STATE{
	MCLK_STATE_DISABLE,
	MCLK_STATE_ENABLE
};

enum MCLK_MODULE{
	MCLK_MODULE_1,
	MCLK_MODULE_2,
	MCLK_MODULE_3,
	MCLK_MODULE_4,
	MCLK_MODULE_MAX_NUM,
};

extern void ISP_MCLK1_EN(BOOL En);
extern void ISP_MCLK2_EN(BOOL En);
extern void ISP_MCLK3_EN(BOOL En);
extern void ISP_MCLK4_EN(BOOL En);

struct mclk{
	void (*pmclk_func[MCLK_MODULE_MAX_NUM])(BOOL);
};

enum IMGSENSOR_RETURN imgsensor_hw_mclk_open(struct IMGSENSOR_HW_DEVICE **pdevice);


#endif

