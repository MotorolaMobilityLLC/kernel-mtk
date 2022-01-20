// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "kd_imgsensor.h"
#include "imgsensor_sensor_list.h"

/* Add Sensor Init function here
 * Note:
 * 1. Add by the resolution from ""large to small"", due to large sensor
 *    will be possible to be main sensor.
 *    This can avoid I2C error during searching sensor.
 * 2. This file should be the same as
 *    mediatek\custom\common\hal\imgsensor\src\sensorlist.cpp
 */
struct IMGSENSOR_SENSOR_LIST
	gimgsensor_sensor_list[MAX_NUM_OF_SUPPORT_SENSOR] = {
#if defined(MOT_LYRIQ_OV50E_MIPI_RAW)
{MOT_LYRIQ_OV50E_SENSOR_ID, SENSOR_DRVNAME_MOT_LYRIQ_OV50E_MIPI_RAW,
       MOT_LYRIQ_OV50E_MIPI_RAW_SensorInit},
#endif
#if defined(MOT_LYRIQ_OV50A_MIPI_RAW)
{MOT_LYRIQ_OV50A_SENSOR_ID, SENSOR_DRVNAME_MOT_LYRIQ_OV50A_MIPI_RAW,
       MOT_LYRIQ_OV50A_MIPI_RAW_SensorInit},
#endif
#if defined(MOT_LYRIQ_OV32B_MIPI_RAW)
{MOT_LYRIQ_OV32B_SENSOR_ID, SENSOR_DRVNAME_MOT_LYRIQ_OV32B_MIPI_RAW,
       MOT_LYRIQ_OV32B_MIPI_RAW_SensorInit},
#endif
#if defined(MOT_LYRIQ_HI1336_MIPI_RAW)
{MOT_LYRIQ_HI1336_SENSOR_ID, SENSOR_DRVNAME_MOT_LYRIQ_HI1336_MIPI_RAW,
       MOT_LYRIQ_HI1336_MIPI_RAW_SensorInit},
#endif
#if defined(IMX766_MIPI_RAW)
{IMX766_SENSOR_ID, SENSOR_DRVNAME_IMX766_MIPI_RAW, IMX766_MIPI_RAW_SensorInit},
#endif
#if defined(MOT_AUSTIN_S5KJN1SQ_MIPI_RAW)
{MOT_AUSTIN_S5KJN1SQ_SENSOR_ID, SENSOR_DRVNAME_MOT_AUSTIN_S5KJN1SQ_MIPI_RAW,
       MOT_AUSTIN_S5KJN1SQ_MIPI_RAW_SensorInit},
#endif
#if defined(MOT_AUSTIN_HI1336_MIPI_RAW)
{MOT_AUSTIN_HI1336_SENSOR_ID, SENSOR_DRVNAME_MOT_AUSTIN_HI1336_MIPI_RAW,
       MOT_AUSTIN_HI1336_MIPI_RAW_SensorInit},
#endif
#if defined(MOT_AUSTIN_GC02M1_MIPI_RAW)
{MOT_AUSTIN_GC02M1_SENSOR_ID, SENSOR_DRVNAME_MOT_AUSTIN_GC02M1_MIPI_RAW,
       MOT_AUSTIN_GC02M1_MIPI_RAW_SensorInit},
#endif
#if defined(MOT_AUSTIN_GC02M1B_MIPI_RAW)
{MOT_AUSTIN_GC02M1B_SENSOR_ID, SENSOR_DRVNAME_MOT_AUSTIN_GC02M1B_MIPI_RAW,
       MOT_AUSTIN_GC02M1B_MIPI_RAW_SensorInit},
#endif
// Kyoto sensors
#if defined(MOT_S5KHM2_MIPI_RAW)
{MOT_S5KHM2_SENSOR_ID, SENSOR_DRVNAME_MOT_S5KHM2_MIPI_RAW, MOT_S5KHM2_MIPI_RAW_SensorInit},
#endif
#if defined(MOT_S5KHM2QTECH_MIPI_RAW)
{MOT_S5KHM2QTECH_SENSOR_ID, SENSOR_DRVNAME_MOT_S5KHM2QTECH_MIPI_RAW,MOT_S5KHM2QTECH_MIPI_RAW_SensorInit},
#endif
#if defined(MOT_OV32B40_MIPI_RAW)
{MOT_OV32B40_SENSOR_ID, SENSOR_DRVNAME_MOT_OV32B40_MIPI_RAW, MOT_OV32B40_MIPI_RAW_SensorInit},
#endif
#if defined(MOT_OV02B1B_MIPI_RAW)
{MOT_OV02B1B_SENSOR_ID, SENSOR_DRVNAME_MOT_OV02B1B_MIPI_RAW, MOT_OV02B1B_MIPI_RAW_SensorInit},
#endif
#if defined(MOT_S5K4H7_MIPI_RAW)
{MOT_S5K4H7_SENSOR_ID, SENSOR_DRVNAME_MOT_S5K4H7_MIPI_RAW, MOT_S5K4H7_MIPI_RAW_SensorInit},
#endif
	/*  ADD sensor driver before this line */
	{0, {0}, NULL}, /* end of list */
};

/* e_add new sensor driver here */

