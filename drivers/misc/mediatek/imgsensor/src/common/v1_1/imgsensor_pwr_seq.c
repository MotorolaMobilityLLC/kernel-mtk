// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016 MediaTek Inc.
 */

#include "kd_imgsensor.h"


#include "imgsensor_hw.h"
#include "imgsensor_cfg_table.h"

/* Legacy design */
struct IMGSENSOR_HW_POWER_SEQ sensor_power_sequence[] = {
#if defined(MOT_LYRIQ_OV50E_MIPI_RAW)
        {
                SENSOR_DRVNAME_MOT_LYRIQ_OV50E_MIPI_RAW,
                {
                        {SensorMCLK, Vol_High, 0},
                        {RST, Vol_Low, 1},
                        {DOVDD, Vol_1800, 0},
                        {AVDD, Vol_2800, 0},
                        {DVDD, Vol_1100, 1},
                        {RST, Vol_High, 5}
                },
        },
#endif
#if defined(MOT_LYRIQ_OV50A_MIPI_RAW)
        {
                SENSOR_DRVNAME_MOT_LYRIQ_OV50A_MIPI_RAW,
                {
                        {SensorMCLK, Vol_High, 0},
                        {RST, Vol_Low, 1},
                        {DOVDD, Vol_1800, 0},
                        {AVDD, Vol_2800, 0},
                        {DVDD, Vol_1100, 1},
                        {RST, Vol_High, 5}
                },
        },
#endif
#if defined(MOT_LYRIQ_OV32B_MIPI_RAW)
	{
		SENSOR_DRVNAME_MOT_LYRIQ_OV32B_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{SensorMCLK, Vol_High, 1},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{DVDD, Vol_1100, 1},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(MOT_LYRIQ_HI1336_MIPI_RAW)
	{
		SENSOR_DRVNAME_MOT_LYRIQ_HI1336_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{SensorMCLK, Vol_High, 1},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{DVDD, Vol_1100, 1},
			{AFVDD, Vol_2800, 1},
			{RST, Vol_High, 2}
		},
	},
#endif

#if defined(IMX766_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX766_MIPI_RAW,
		{
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 1},
			{AVDD, Vol_2800, 3},
#ifdef CONFIG_REGULATOR_RT5133
			{AVDD1, Vol_1800, 0},
#endif
			{AFVDD, Vol_2800, 3},
			{DVDD, Vol_1100, 4},
			{DOVDD, Vol_1800, 1},
			{SensorMCLK, Vol_High, 6},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 5}
		},
	},
#endif
#ifdef CONFIG_CAMERA_DVDD_SWITCH
//+EKSAIPAN,add it for saipan main and front camera bringup
#if defined(SAIPAN_QTECH_HI4821Q_MIPI_RAW)
	{
		SENSOR_DRVNAME_SAIPAN_QTECH_HI4821Q_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{AVDD, Vol_High, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD_1V2_IN, Vol_High, 1},
			{DVDD_1V1, Vol_Low, 1},
			{DVDD, Vol_High, 1},
			{SensorMCLK, Vol_High, 10},
			{RST, Vol_High, 10},
		},
	},
#endif
#if defined(SAIPAN_SHINE_HI846_MIPI_RAW)
	{
		SENSOR_DRVNAME_SAIPAN_SHINE_HI846_MIPI_RAW,
		{
			{DOVDD, Vol_1800, 1},
			{DVDD_1V1, Vol_High, 1},
			{DVDD_1V2, Vol_Low, 1},
			{DVDD, Vol_High, 1},
			{AVDD, Vol_High, 1},
			{SensorMCLK, Vol_High, 5},
			{RST, Vol_Low, 10},
			{RST, Vol_High, 1},
		},
	},
#endif
#if defined(SAIPAN_DMEGC_HI1336_MIPI_RAW)
	{
		SENSOR_DRVNAME_SAIPAN_DMEGC_HI1336_MIPI_RAW,
		{
			{DOVDD, Vol_1800, 1},
			{DVDD_1V1, Vol_High, 1},
			{DVDD_1V2, Vol_Low, 1},
			{DVDD, Vol_High, 1},
			{AVDD, Vol_High, 1},
			{SensorMCLK, Vol_High, 10},
			{RST, Vol_Low, 10},
			{RST, Vol_High, 1},
		},
	},
#endif
#if defined(SAIPAN_CXT_GC02M1B_MIPI_MONO)
	{
		SENSOR_DRVNAME_SAIPAN_CXT_GC02M1B_MIPI_MONO,
		{
			{RST, Vol_Low, 15},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{RST, Vol_High, 1},
			{SensorMCLK, Vol_High, 1}
		},
	},
#endif
#if defined(SAIPAN_CXT_GC02M1_MIPI_RAW)
	{
		SENSOR_DRVNAME_SAIPAN_CXT_GC02M1_MIPI_RAW,
		{
			{RST, Vol_Low, 15},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{RST, Vol_High, 1},
			{SensorMCLK, Vol_High, 1}
		},
	},
#endif
//-EKSAIPAN,add it for saipan main and front camera bringup
#endif
#if defined(MOT_AUSTIN_S5KJN1SQ_MIPI_RAW)
	{
		SENSOR_DRVNAME_MOT_AUSTIN_S5KJN1SQ_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DVDD, Vol_1100, 0},
			{DOVDD, Vol_1800, 2},
			{AFVDD, Vol_2800, 1},
			{AVDD, Vol_2800, 3},
			{RST, Vol_High, 1},
			{SensorMCLK, Vol_High, 9},
		},
	},
#endif
#if defined(MOT_AUSTIN_HI1336_MIPI_RAW)
	{
		SENSOR_DRVNAME_MOT_AUSTIN_HI1336_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{SensorMCLK, Vol_High, 1},
			{PDN, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{DVDD, Vol_1100, 1},
			{PDN, Vol_High, 2},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(MOT_AUSTIN_GC02M1B_MIPI_RAW)
       {
               SENSOR_DRVNAME_MOT_AUSTIN_GC02M1B_MIPI_RAW,
               {
                       {RST, Vol_Low, 1},
 		       {SensorMCLK, Vol_High, 2},
                       {DOVDD, Vol_1800, 1},
                       {AVDD, Vol_2800, 1},
                       {RST, Vol_High, 2},
               },
       },
#endif
#if defined(MOT_AUSTIN_GC02M1_MIPI_RAW)
	{
		SENSOR_DRVNAME_MOT_AUSTIN_GC02M1_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{SensorMCLK, Vol_High, 2},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 0},
			{RST, Vol_High, 2}
		},
	},
#endif
// kyoto sensors
#if defined(MOT_S5KHM2_MIPI_RAW)
       {
               SENSOR_DRVNAME_MOT_S5KHM2_MIPI_RAW,
               {
                       {RST, Vol_Low, 1},
                       {SensorMCLK, Vol_High, 1},
                       {DOVDD, Vol_1800, 1},
                       {AVDD, Vol_High, 1},
                       {DVDD, Vol_High, 1},
                       {RST, Vol_High, 5},
               },
       },
#endif
#if defined(MOT_S5KHM2QTECH_MIPI_RAW)
        {
                SENSOR_DRVNAME_MOT_S5KHM2QTECH_MIPI_RAW,
                {
                        {RST, Vol_Low, 1},
                        {SensorMCLK, Vol_High, 1},
                        {DOVDD, Vol_1800, 1},
                        {AVDD, Vol_High, 1},
                        {DVDD, Vol_High, 1},
                        {RST, Vol_High, 5},
                },
        },
#endif
#if defined(MOT_OV32B40_MIPI_RAW)
       {
               SENSOR_DRVNAME_MOT_OV32B40_MIPI_RAW,
               {
                       {RST, Vol_Low, 1},
                       {SensorMCLK, Vol_High, 1},
                       {AVDD, Vol_High, 1},
                       {DOVDD, Vol_1800, 1},
                       {DVDD, Vol_High, 1},
                       {RST, Vol_High, 5},
               },
       },
#endif
#if defined(MOT_OV02B1B_MIPI_RAW)
       {
               SENSOR_DRVNAME_MOT_OV02B1B_MIPI_RAW,
               {
                       {RST, Vol_Low, 1},
                       {DOVDD, Vol_1800, 1},
                       {AVDD, Vol_High, 11},
                       {SensorMCLK, Vol_High, 1},
                       {RST, Vol_High, 5},
               },
       },
#endif
#if defined(MOT_S5K4H7_MIPI_RAW)
       {
               SENSOR_DRVNAME_MOT_S5K4H7_MIPI_RAW,
               {
                       {RST, Vol_Low, 1},
                       {SensorMCLK, Vol_High, 1},
                       {AVDD, Vol_High, 1},
                       {DVDD, Vol_High, 1},
                       {DOVDD, Vol_1800, 1},
                       {RST, Vol_High, 5},
               },
       },
#endif
	{NULL,},
};

