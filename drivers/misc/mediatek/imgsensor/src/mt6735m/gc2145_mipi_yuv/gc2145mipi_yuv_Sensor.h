/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/*****************************************************************************
 *
 * Filename:
 * ---------
 *   sensor.h
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *   Header file of Sensor driver
 *
 *
 * Author:
 * -------
 *   Leo Lee
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * [GC2145mipiYUV V1.0.0]
 * 8.17.2012 Leo.Lee
 * .First Release
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by GalaxyCoreinc. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
/* SENSOR FULL SIZE */
#ifndef __GC2145_SENSOR_H
#define __GC2145_SENSOR_H

typedef enum _GC2145MIPI_OP_TYPE_ {
        GC2145MIPI_MODE_NONE,
        GC2145MIPI_MODE_PREVIEW,
        GC2145MIPI_MODE_CAPTURE,
        GC2145MIPI_MODE_QCIF_VIDEO,
        GC2145MIPI_MODE_CIF_VIDEO,
        GC2145MIPI_MODE_QVGA_VIDEO
    } GC2145MIPI_OP_TYPE;

extern GC2145MIPI_OP_TYPE GC2145MIPI_g_iGC2145MIPI_Mode;

/* START GRAB PIXEL OFFSET */
#define IMAGE_SENSOR_START_GRAB_X		        2	// 0 or 1 recommended
#define IMAGE_SENSOR_START_GRAB_Y		        2	// 0 or 1 recommended

/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
#define GC2145MIPI_FULL_PERIOD_PIXEL_NUMS  (1940)  // default pixel#(w/o dummy pixels) in UXGA mode
#define GC2145MIPI_FULL_PERIOD_LINE_NUMS   (1238)  // default line#(w/o dummy lines) in UXGA mode
#define GC2145MIPI_PV_PERIOD_PIXEL_NUMS    (970)  // default pixel#(w/o dummy pixels) in SVGA mode
#define GC2145MIPI_PV_PERIOD_LINE_NUMS     (670)   // default line#(w/o dummy lines) in SVGA mode

/* SENSOR EXPOSURE LINE LIMITATION */
#define GC2145MIPI_FULL_EXPOSURE_LIMITATION    (1236)
#define GC2145MIPI_PV_EXPOSURE_LIMITATION      (671)

/* SENSOR FULL SIZE */
#define GC2145MIPI_IMAGE_SENSOR_FULL_WIDTH	   (1600)
#define GC2145MIPI_IMAGE_SENSOR_FULL_HEIGHT	  (1200)

/* SENSOR PV SIZE */
#define GC2145MIPI_IMAGE_SENSOR_PV_WIDTH   (800)
#define GC2145MIPI_IMAGE_SENSOR_PV_HEIGHT  (600)

#define GC2145MIPI_VIDEO_QCIF_WIDTH   (176)
#define GC2145MIPI_VIDEO_QCIF_HEIGHT  (144)

/* SENSOR READ/WRITE ID */
#define GC2145MIPI_WRITE_ID	        0x78
#define GC2145MIPI_READ_ID		0x79


/* SENSOR CHIP VERSION */
//#define GC2145MIPI_SENSOR_ID							0x2145


//s_add for porting
//s_add for porting
//s_add for porting

//export functions
UINT32 GC2145MIPIOpen(void);
UINT32 GC2145MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 GC2145MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 GC2145MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 GC2145MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 GC2145MIPIClose(void);


//e_add for porting
//e_add for porting
//e_add for porting
extern void kdSetI2CSpeed(u16 i2cSpeed);


#endif /* __SENSOR_H */
