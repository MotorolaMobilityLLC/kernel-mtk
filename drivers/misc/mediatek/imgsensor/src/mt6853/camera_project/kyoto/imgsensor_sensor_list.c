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

#include "kd_imgsensor.h"
#include "imgsensor_sensor_list.h"

struct IMGSENSOR_SENSOR_LIST
	gimgsensor_sensor_list[MAX_NUM_OF_SUPPORT_SENSOR] = {
#if defined(MOT_S5KHM2_MIPI_RAW)
{MOT_S5KHM2_SENSOR_ID, SENSOR_DRVNAME_MOT_S5KHM2_MIPI_RAW,MOT_S5KHM2_MIPI_RAW_SensorInit},
#endif
#if defined(MOT_S5KHM2QTECH_MIPI_RAW)
{MOT_S5KHM2QTECH_SENSOR_ID, SENSOR_DRVNAME_MOT_S5KHM2QTECH_MIPI_RAW,MOT_S5KHM2QTECH_MIPI_RAW_SensorInit},
#endif
#if defined(MOT_OV32B40_MIPI_RAW)
{MOT_OV32B40_SENSOR_ID, SENSOR_DRVNAME_MOT_OV32B40_MIPI_RAW,MOT_OV32B40_MIPI_RAW_SensorInit},
#endif
#if defined(MOT_OV02B1B_MIPI_RAW)
{MOT_OV02B1B_SENSOR_ID, SENSOR_DRVNAME_MOT_OV02B1B_MIPI_RAW,MOT_OV02B1B_MIPI_RAW_SensorInit},
#endif
#if defined(MOT_S5K4H7_MIPI_RAW)
{MOT_S5K4H7_SENSOR_ID, SENSOR_DRVNAME_MOT_S5K4H7_MIPI_RAW,MOT_S5K4H7_MIPI_RAW_SensorInit},
#endif
	{0, {0}, NULL}, /* end of list */
};

