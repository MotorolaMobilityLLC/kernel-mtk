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
 * 2. This should be the same as
 *     mediatek\custom\common\hal\imgsensor\src\sensorlist.cpp
 */
struct IMGSENSOR_INIT_FUNC_LIST kdSensorList[MAX_NUM_OF_SUPPORT_SENSOR] = {
#if defined(S5KJN1_QT_P410AE_MIPI_RAW)
	{S5KJN1_QT_P410AE_SENSOR_ID,
	SENSOR_DRVNAME_S5KJN1_QT_P410AE_MIPI_RAW,
	S5KJN1_QT_P410AE_MIPI_RAW_SensorInit},
#endif
#if defined(HI1336_QTECH_P410AE_MIPI_RAW)
	{HI1336_QTECH_P410AE_SENSOR_ID,
	SENSOR_DRVNAME_HI1336_QTECH_P410AE_MIPI_RAW,
	HI1336_QTECH_P410AE_MIPI_RAW_SensorInit},
#endif
#if defined(HI1634_QT_P411BE_MIPI_RAW)
       {HI1634_QT_P411BE_SENSOR_ID,
       SENSOR_DRVNAME_HI1634_QT_P411BE_MIPI_RAW,
       HI1634_QT_P411BE_MIPI_RAW_SensorInit},
#endif
#if defined(S5K4H7SUB_QT_P410AE_MIPI_RAW)
	{S5K4H7SUB_QT_P410AE_SENSOR_ID,
	SENSOR_DRVNAME_S5K4H7SUB_QT_P410AE_MIPI_RAW,
	S5K4H7SUB_QT_P410AE_MIPI_RAW_SensorInit},
#endif
#if defined(OV16A1Q_QT_P410AE_MIPI_RAW)
	{OV16A1Q_QT_P410AE_SENSOR_ID,
	SENSOR_DRVNAME_OV16A1Q_QT_P410AE_MIPI_RAW,
	OV16A1Q_QT_P410AE_MIPI_RAW_SensorInit},
#endif
#if defined(HI1634_QT_P410AE_MIPI_RAW)
       {HI1634_QT_P410AE_SENSOR_ID,
       SENSOR_DRVNAME_HI1634_QT_P410AE_MIPI_RAW,
       HI1634_QT_P410AE_MIPI_RAW_SensorInit},
#endif
#if defined(S5K4H7WIDE_QT_P410AE_MIPI_RAW)
	{S5K4H7WIDE_QT_P410AE_SENSOR_ID,
	SENSOR_DRVNAME_S5K4H7WIDE_QT_P410AE_MIPI_RAW,
	S5K4H7WIDE_QT_P410AE_MIPI_RAW_SensorInit},
#endif
#if defined(GC2M1B_TSP_P410AE_MIPI_RAW)
	{GC2M1B_TSP_P410AE_SENSOR_ID,
	SENSOR_DRVNAME_GC2M1B_TSP_P410AE_MIPI_RAW,
	GC2M1B_TSP_P410AE_MIPI_RAW_SensorInit},
#endif
#if defined(GC02M1_TSP_P410AE_MIPI_RAW)
	{GC02M1_TSP_P410AE_SENSOR_ID,
	SENSOR_DRVNAME_GC02M1_TSP_P410AE_MIPI_RAW,
	GC02M1_TSP_P410AE_MIPI_RAW_SensorInit},
#endif
	/*  ADD sensor driver before this line */
	{0, {0}, NULL}, /* end of list */
};
/* e_add new sensor driver here */

