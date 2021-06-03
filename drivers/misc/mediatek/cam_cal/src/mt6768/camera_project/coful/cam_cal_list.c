/*
 * Copyright (C) 2018 MediaTek Inc.
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
#include <linux/kernel.h>
#include "cam_cal_list.h"
#include "eeprom_i2c_common_driver.h"
#include "eeprom_i2c_custom_driver.h"
#include "kd_imgsensor.h"
extern unsigned int S5K4H7_read_region(struct i2c_client *client, unsigned int addr,
			unsigned char *data, unsigned int size);
struct stCAM_CAL_LIST_STRUCT g_camCalList[] = {
	/*Below is commom sensor */
	{MOT_COFUL_S5KJN1_QTECH_ID, 0xA0, Common_read_region},
	{MOT_CORFU_HI1336_OFILM_ID, 0xA2, Common_read_region},
        {MOT_COFUL_S5K4H7_QTECH_ID, 0x5A, S5K4H7_read_region},
        {MOT_COFUL_GC02M1_TSP_ID, 0x6e, GC02M1_read_region},
	/*  ADD before this line */
	{0, 0, 0}       /*end of list */
};

unsigned int cam_cal_get_sensor_list(
	struct stCAM_CAL_LIST_STRUCT **ppCamcalList)
{
	if (ppCamcalList == NULL)
		return 1;

	*ppCamcalList = &g_camCalList[0];
	return 0;
}


