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
 *   gc0310yuv_Sensor.c
 *
 * Project:
 * --------
 *   MAUI
 *
 * Description:
 * ------------
 *   Image sensor driver function
 *   V1.2.3
 *
 * Author:
 * -------
 *   Leo
 *
 *=============================================================
 *             HISTORY
 * Below this line, this part is controlled by GCoreinc. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Log$
 * 2012.02.29  kill bugs
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by GCoreinc. DO NOT MODIFY!!
 *=============================================================
 ******************************************************************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/types.h>

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "gc0310_yuv_Sensor.h"


static DEFINE_SPINLOCK(GC0310_drv_lock);


#define GC0310YUV_DEBUG

#ifdef GC0310YUV_DEBUG
#define SENSORDB LOG_INF
#else
#define SENSORDB(x,...)
#endif

/****************************Modify Following Strings for Debug****************************/
#define PFX "GC0310SPIYUV"
#define LOG_1 LOG_INF("GC0310,SERIAL CAM\n")
/****************************   Modify end    *******************************************/

#define LOG_INF(format, args...)    pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)

#define GC0310_TEST_PATTERN_CHECKSUM (0xb6e34f39)
#define GC0310_BANDING_50HZ    0
#define GC0310_BANDING_60HZ    1
#define I2C_SPEED 200
kal_bool GC0310_night_mode_enable = KAL_FALSE;
kal_uint16 GC0310_CurStatus_AWB = 0;
kal_bool GC0310_banding_state = GC0310_BANDING_50HZ;      // 0~50hz; 1~60hz

static void GC0310_awb_enable(kal_bool enalbe);

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern void kdSetI2CSpeed(u16 i2cSpeed);


kal_uint16 GC0310_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
    char puSendCmd[2] = {(char)(addr & 0xFF) , (char)(para & 0xFF)};
	kdSetI2CSpeed(I2C_SPEED);
    iWriteRegI2C(puSendCmd , 2, GC0310_WRITE_ID);

}
/*kal_uint16 GC0310_read_cmos_sensor(kal_uint8 addr)
{
    kal_uint16 get_byte=0;
    char puSendCmd = { (char)(addr & 0xFF) };
    iReadRegI2C(&puSendCmd , 1, (u8*)&get_byte, 1, GC0310_READ_ID);

    return get_byte;
}*/

kal_uint16 CamWriteCmosSensor(kal_uint8 addr, kal_uint8 para)
{
    char puSendCmd[2] = {(char)(addr & 0xFF) , (char)(para & 0xFF)};
	kdSetI2CSpeed(I2C_SPEED);
    iWriteRegI2C(puSendCmd , 2, GC0310_WRITE_ID);
}

static void write_cmos_sensor2(kal_uint32 addr, kal_uint32 para)
{
    char pu_send_cmd[2] = {(char)(addr & 0xFF), (char)(para & 0xFF)};
	kdSetI2CSpeed(I2C_SPEED);
    iWriteRegI2C(pu_send_cmd, 2, GC0310_WRITE_ID);

}



static kal_uint16 GC0310_read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;

    char pu_send_cmd[1] = {(char)(addr & 0xFF) };
	kdSetI2CSpeed(I2C_SPEED);
    iReadRegI2C(pu_send_cmd, 1, (u8*)&get_byte, 1, GC0310_WRITE_ID);

    return get_byte;

}

/*******************************************************************************
 * // Adapter for Winmo typedef
 ********************************************************************************/
#define WINMO_USE 0

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT

kal_bool   GC0310_MPEG4_encode_mode = KAL_FALSE;
kal_uint16 GC0310_dummy_pixels = 0, GC0310_dummy_lines = 0;
kal_bool   GC0310_NIGHT_MODE = KAL_FALSE;

kal_uint32 MINFramerate = 0;
kal_uint32 MAXFramerate = 0;

kal_uint32 GC0310_isp_master_clock;
static kal_uint32 GC0310_g_fPV_PCLK = 30 * 1000000;

kal_uint8 GC0310_sensor_write_I2C_address = GC0310_WRITE_ID;
kal_uint8 GC0310_sensor_read_I2C_address = GC0310_READ_ID;

UINT8 GC0310PixelClockDivider=0;

MSDK_SENSOR_CONFIG_STRUCT GC0310SensorConfigData;

#define GC0310_SET_PAGE0    GC0310_write_cmos_sensor(0xfe, 0x00)
#define GC0310_SET_PAGE1    GC0310_write_cmos_sensor(0xfe, 0x01)
#define GC0310_SET_PAGE2    GC0310_write_cmos_sensor(0xfe, 0x02)
#define GC0310_SET_PAGE3    GC0310_write_cmos_sensor(0xfe, 0x03)

/*************************************************************************
 * FUNCTION
 *  GC0310_SetShutter
 *
 * DESCRIPTION
 *  This function set e-shutter of GC0310 to change exposure time.
 *
 * PARAMETERS
 *   iShutter : exposured lines
 *
 * RETURNS
 *  None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC0310_Set_Shutter(kal_uint16 iShutter)
{
} /* Set_GC0310_Shutter */


/*************************************************************************
 * FUNCTION
 *  GC0310_read_Shutter
 *
 * DESCRIPTION
 *  This function read e-shutter of GC0310 .
 *
 * PARAMETERS
 *  None
 *
 * RETURNS
 *  None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint16 GC0310_Read_Shutter(void)
{
        kal_uint8 temp_reg1, temp_reg2;
    kal_uint16 shutter;

    temp_reg1 = GC0310_read_cmos_sensor(0x04);
    temp_reg2 = GC0310_read_cmos_sensor(0x03);

    shutter = (temp_reg1 & 0xFF) | (temp_reg2 << 8);

    return shutter;
} /* GC0310_read_shutter */

static void GC0310_Set_Mirrorflip(kal_uint8 image_mirror)
{
    LOG_INF("image_mirror = %d\n", image_mirror);

    switch (image_mirror)
    {
        case IMAGE_NORMAL://IMAGE_NORMAL:
            GC0310_write_cmos_sensor(0x17,0x14);//bit[1][0]
    //      write_cmos_sensor(0x92,0x03);
    //      write_cmos_sensor(0x94,0x0b);
            break;
        case IMAGE_H_MIRROR://IMAGE_H_MIRROR:
            GC0310_write_cmos_sensor(0x17,0x15);
    //      GC2355_write_cmos_sensor(0x92,0x03);
    //      GC2355_write_cmos_sensor(0x94,0x0b);
            break;
        case IMAGE_V_MIRROR://IMAGE_V_MIRROR:
            GC0310_write_cmos_sensor(0x17,0x16);
    //      GC2355_write_cmos_sensor(0x92,0x02);
    //      GC2355_write_cmos_sensor(0x94,0x0b);
            break;
        case IMAGE_HV_MIRROR://IMAGE_HV_MIRROR:
            GC0310_write_cmos_sensor(0x17,0x17);
    //      GC2355_write_cmos_sensor(0x92,0x02);
    //      GC2355_write_cmos_sensor(0x94,0x0b);
            break;
    }


}

static void GC0310_set_AE_mode(kal_bool AE_enable)
{

    LOG_INF("[GC0310]enter GC0310_set_AE_mode function, AE_enable = %d\n ", AE_enable);
    if (AE_enable == KAL_TRUE)
    {
        // turn on AEC/AGC
            LOG_INF("[GC0310]enter GC0310_set_AE_mode function 1\n ");
            GC0310_write_cmos_sensor(0xfe, 0x00);
            GC0310_write_cmos_sensor(0x4f, 0x01);
    }
    else
    {
        // turn off AEC/AGC
            LOG_INF("[GC0310]enter GC0310_set_AE_mode function 2\n ");
            GC0310_write_cmos_sensor(0xfe, 0x00);
            GC0310_write_cmos_sensor(0x4f, 0x00);
    }
}


void GC0310_set_contrast(UINT16 para)
{
    LOG_INF("[GC0310]CONTROLFLOW enter GC0310_set_contrast function:\n ");
#if 1
    switch (para)
    {
        case ISP_CONTRAST_LOW:
            GC0310_write_cmos_sensor(0xd3, 0x30);
            break;
        case ISP_CONTRAST_HIGH:
            GC0310_write_cmos_sensor(0xd3, 0x60);
            break;
        case ISP_CONTRAST_MIDDLE:
        default:
            GC0310_write_cmos_sensor(0xd3, 0x40);
            break;

    }
    LOG_INF("[GC0310]exit GC0310_set_contrast function:\n ");
    return;
#endif
}

UINT32 GC0310_MIPI_SetMaxFramerateByScenario(
  MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate)
{
    LOG_INF("scenarioId = %d\n", scenarioId);
}


UINT32 GC0310SetTestPatternMode(kal_bool bEnable)
{
    LOG_INF("[GC0310SetTestPatternMode]test pattern bEnable:=%d\n",bEnable);

    if(bEnable)
    {
        GC0310_write_cmos_sensor(0xfe,0x00);
        GC0310_write_cmos_sensor(0x40,0x08);
        GC0310_write_cmos_sensor(0x41,0x00);
        GC0310_write_cmos_sensor(0x42,0x00);
        GC0310_write_cmos_sensor(0x4f,0x00);
        GC0310_write_cmos_sensor(0xfe,0x00);
        GC0310_write_cmos_sensor(0x03,0x03);
        GC0310_write_cmos_sensor(0x04,0x9c);
        GC0310_write_cmos_sensor(0x71,0x20);
        GC0310_write_cmos_sensor(0x72,0x40);
        GC0310_write_cmos_sensor(0x73,0x80);
        GC0310_write_cmos_sensor(0x74,0x80);
        GC0310_write_cmos_sensor(0x75,0x80);
        GC0310_write_cmos_sensor(0x76,0x80);
        GC0310_write_cmos_sensor(0x7a,0x80);
        GC0310_write_cmos_sensor(0x7b,0x80);
        GC0310_write_cmos_sensor(0x7c,0x80);
        GC0310_write_cmos_sensor(0x77,0x40);
        GC0310_write_cmos_sensor(0x78,0x40);
        GC0310_write_cmos_sensor(0x79,0x40);
        GC0310_write_cmos_sensor(0xfe,0x00);
        GC0310_write_cmos_sensor(0xfe,0x01);
        GC0310_write_cmos_sensor(0x12,0x00);
        GC0310_write_cmos_sensor(0x13,0x30);
        GC0310_write_cmos_sensor(0x44,0x00);
        GC0310_write_cmos_sensor(0x45,0x00);
        GC0310_write_cmos_sensor(0xfe,0x00);
        GC0310_write_cmos_sensor(0xd0,0x40);
        GC0310_write_cmos_sensor(0xd1,0x20);
        GC0310_write_cmos_sensor(0xd2,0x20);
        GC0310_write_cmos_sensor(0xd3,0x40);
        GC0310_write_cmos_sensor(0xd5,0x00);
        GC0310_write_cmos_sensor(0xd8,0x00);
        GC0310_write_cmos_sensor(0xdd,0x00);
        GC0310_write_cmos_sensor(0xde,0x00);
        GC0310_write_cmos_sensor(0xe4,0x00);
        GC0310_write_cmos_sensor(0xeb,0x00);
        GC0310_write_cmos_sensor(0xa4,0x00);
        GC0310_write_cmos_sensor(0x4c,0x21);

        GC0310_write_cmos_sensor(0xfe,0x00);


    }
    else
    {
        GC0310_write_cmos_sensor(0xfe, 0x00);
        GC0310_write_cmos_sensor(0x4c, 0x20);
        GC0310_write_cmos_sensor(0xfe, 0x00);
    }

    return ERROR_NONE;
}


void GC0310_set_brightness(UINT16 para)
{

    LOG_INF("[GC0310]CONTROLFLOW enter GC0310_set_brightness function:\n ");
#if 1
    //return;
    switch (para)
    {
        case ISP_BRIGHT_LOW:
            GC0310_SET_PAGE0;
            GC0310_write_cmos_sensor(0xd5, 0xc0);
        break;
        case ISP_BRIGHT_HIGH:
            GC0310_SET_PAGE0;
            GC0310_write_cmos_sensor(0xd5, 0x40);
            break;
        case ISP_BRIGHT_MIDDLE:
        default:
            GC0310_SET_PAGE0;
            GC0310_write_cmos_sensor(0xd5, 0x00);
        break;
    }
    LOG_INF("[GC0310]exit GC0310_set_brightness function:\n ");
    return;
#endif
}
void GC0310_set_saturation(UINT16 para)
{
    LOG_INF("[GC0310]CONTROLFLOW enter GC0310_set_saturation function:\n ");

    switch (para)
    {
        case ISP_SAT_HIGH:
            GC0310_write_cmos_sensor(0xfe, 0x00);
            GC0310_write_cmos_sensor(0xd1, 0x45);
            GC0310_write_cmos_sensor(0xd2, 0x45);
            GC0310_write_cmos_sensor(0xfe, 0x00);
            break;
        case ISP_SAT_LOW:
            GC0310_write_cmos_sensor(0xfe, 0x00);
            GC0310_write_cmos_sensor(0xd1, 0x28);
            GC0310_write_cmos_sensor(0xd2, 0x28);
            GC0310_write_cmos_sensor(0xfe, 0x00);
            break;
        case ISP_SAT_MIDDLE:
        default:
            GC0310_write_cmos_sensor(0xfe, 0x00);
            GC0310_write_cmos_sensor(0xd1, 0x34);
            GC0310_write_cmos_sensor(0xd2, 0x34);
            GC0310_write_cmos_sensor(0xfe, 0x00);
            break;
        //  return KAL_FALSE;
        //  break;
    }
    LOG_INF("[GC0310]exit GC0310_set_saturation function:\n ");
     return;

}

void GC0310_set_iso(UINT16 para)
{

    LOG_INF("[GC0310]CONTROLFLOW GC0310_set_iso AEC p0:0x4f %d: shutter %d\n",GC0310_read_cmos_sensor(0x4f),GC0310_Read_Shutter());
    switch (para)
    {
        case AE_ISO_100:
             //ISO 100
            GC0310_write_cmos_sensor(0xfe, 0x01);
            GC0310_write_cmos_sensor(0x1f, 0x20);
            GC0310_write_cmos_sensor(0x20, 0x40);
            GC0310_write_cmos_sensor(0x44, 0x00);
            GC0310_write_cmos_sensor(0xfe, 0x00);
            break;
        case AE_ISO_200:
             //ISO 200
            GC0310_write_cmos_sensor(0xfe, 0x01);
            GC0310_write_cmos_sensor(0x1f, 0x20);
            GC0310_write_cmos_sensor(0x20, 0x80);
            GC0310_write_cmos_sensor(0x44, 0x02);
            GC0310_write_cmos_sensor(0xfe, 0x00);
            break;
        case AE_ISO_400:
             //ISO 400
            GC0310_write_cmos_sensor(0xfe, 0x01);
            GC0310_write_cmos_sensor(0x1f, 0x30);
            GC0310_write_cmos_sensor(0x20, 0xc0);
            GC0310_write_cmos_sensor(0x44, 0x02);
            GC0310_write_cmos_sensor(0xfe, 0x00);
            break;
        case AE_ISO_AUTO:
        default:
            GC0310_write_cmos_sensor(0xfe, 0x01);
            GC0310_write_cmos_sensor(0x1f, 0x30);
            GC0310_write_cmos_sensor(0x20, 0xc0);
            GC0310_write_cmos_sensor(0x44, 0x03);
            GC0310_write_cmos_sensor(0xfe, 0x00);
            break;
    }
    return;
}



void GC0310StreamOn(void)
{
return;
    GC0310_write_cmos_sensor(0xfe,0x03);
    GC0310_write_cmos_sensor(0xf3,0x83);
    GC0310_write_cmos_sensor(0xfe,0x00);
    Sleep(50);
}

void GC0310_MIPI_GetDelayInfo(uintptr_t delayAddr)
{
    SENSOR_DELAY_INFO_STRUCT* pDelayInfo = (SENSOR_DELAY_INFO_STRUCT*)delayAddr;
    pDelayInfo->InitDelay = 2;
    pDelayInfo->EffectDelay = 2;
    pDelayInfo->AwbDelay = 2;
    pDelayInfo->AFSwitchDelayFrame = 50;
    pDelayInfo->ContrastDelay = 1;
    pDelayInfo->EvDelay = 0;
    pDelayInfo->BrightDelay = 1;
    pDelayInfo->SatDelay = 0;
}

UINT32 GC0310_MIPI_GetDefaultFramerateByScenario(
  MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate)
{
    switch (scenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
             *pframeRate = 300;
             break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
             *pframeRate = 300;
             break;
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added
             *pframeRate = 300;
             break;
        default:
             *pframeRate = 300;
          break;
    }

  return ERROR_NONE;
}

void GC0310_MIPI_SetMaxMinFps(UINT32 u2MinFrameRate, UINT32 u2MaxFrameRate)
{
    LOG_INF("GC0310_MIPI_SetMaxMinFps+ :FrameRate= %d %d\n",u2MinFrameRate,u2MaxFrameRate);
    spin_lock(&GC0310_drv_lock);
    MINFramerate = u2MinFrameRate;
    MAXFramerate = u2MaxFrameRate;
    spin_unlock(&GC0310_drv_lock);
    return;
}

void GC0310_3ACtrl(ACDK_SENSOR_3A_LOCK_ENUM action)
{
    LOG_INF("[GC0329]enter ACDK_SENSOR_3A_LOCK_ENUM function:action=%d\n",action);
   switch (action)
   {
      case SENSOR_3A_AE_LOCK:
          GC0310_set_AE_mode(KAL_FALSE);
      break;
      case SENSOR_3A_AE_UNLOCK:
          GC0310_set_AE_mode(KAL_TRUE);
      break;

      case SENSOR_3A_AWB_LOCK:
          GC0310_awb_enable(KAL_FALSE);
      break;

      case SENSOR_3A_AWB_UNLOCK:
           if (((AWB_MODE_OFF == GC0310_CurStatus_AWB) ||
                (AWB_MODE_AUTO == GC0310_CurStatus_AWB)))
            {
                     GC0310_awb_enable(KAL_TRUE);
            }
      break;
      default:
        break;
   }
   LOG_INF("[GC0329]exit ACDK_SENSOR_3A_LOCK_ENUM function:action=%d\n",action);
   return;
}


void GC0310_MIPI_GetExifInfo(uintptr_t exifAddr)
{
    SENSOR_EXIF_INFO_STRUCT* pExifInfo = (SENSOR_EXIF_INFO_STRUCT*)exifAddr;
    pExifInfo->FNumber = 28;
	GC0310_write_cmos_sensor(0xfe, 0x00);

	int preGain =GC0310_read_cmos_sensor(0x71);
	int postGain =GC0310_read_cmos_sensor(0x72);


	if(!(postGain>0x40)){
		pExifInfo->RealISOValue = AE_ISO_100;
	}else if(!(postGain>0x80)){
		pExifInfo->RealISOValue = AE_ISO_200;
	}else if(!(postGain>0xc0)){
		pExifInfo->RealISOValue = AE_ISO_400;
	}else {
		pExifInfo->RealISOValue = AE_ISO_400;
	}
	LOG_INF("[GC0310]GC0310_MIPI_GetExifInfo preGain=0x%x postGain=0x%x iso=%d\n",preGain,postGain,pExifInfo->RealISOValue);
//    pExifInfo->AEISOSpeed = GC0310_Driver.isoSpeed;
//    pExifInfo->AWBMode = S5K4ECGX_Driver.awbMode;
//    pExifInfo->CapExposureTime = S5K4ECGX_Driver.capExposureTime;
//    pExifInfo->FlashLightTimeus = 0;
//   pExifInfo->RealISOValue = (S5K4ECGX_MIPI_ReadGain()*57) >> 8;
        //S5K4ECGX_Driver.isoSpeed;
}

#if 0
void GC0310_MIPI_get_AEAWB_lock(uintptr_t pAElockRet32, uintptr_t pAWBlockRet32)
{
    *pAElockRet32 = 1;
    *pAWBlockRet32 = 1;
    LOG_INF("[GC0310]GetAEAWBLock,AE=%d ,AWB=%d\n,",(int)*pAElockRet32, (int)*pAWBlockRet32);
}
#endif

/*************************************************************************
 * FUNCTION
 *  GC0310_write_reg
 *
 * DESCRIPTION
 *  This function set the register of GC0310.
 *
 * PARAMETERS
 *  addr : the register index of GC0310
 *  para : setting parameter of the specified register of GC0310
 *
 * RETURNS
 *  None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC0310_write_reg(kal_uint32 addr, kal_uint32 para)
{
    GC0310_write_cmos_sensor(addr, para);
} /* GC0310_write_reg() */


/*************************************************************************
 * FUNCTION
 *  GC0310_read_cmos_sensor
 *
 * DESCRIPTION
 *  This function read parameter of specified register from GC0310.
 *
 * PARAMETERS
 *  addr : the register index of GC0310
 *
 * RETURNS
 *  the data that read from GC0310
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint32 GC0310_read_reg(kal_uint32 addr)
{
    return GC0310_read_cmos_sensor(addr);
} /* OV7670_read_reg() */


/*************************************************************************
* FUNCTION
*   GC0310_awb_enable
*
* DESCRIPTION
*   This function enable or disable the awb (Auto White Balance).
*
* PARAMETERS
*   1. kal_bool : KAL_TRUE - enable awb, KAL_FALSE - disable awb.
*
* RETURNS
*   kal_bool : It means set awb right or not.
*
*************************************************************************/
static void GC0310_awb_enable(kal_bool enalbe)
{
    kal_uint16 temp_AWB_reg = 0;

    GC0310_write_cmos_sensor(0xfe, 0x00);
    temp_AWB_reg = GC0310_read_cmos_sensor(0x42);

    if (enalbe)
    {
        GC0310_write_cmos_sensor(0x42, (temp_AWB_reg |0x02));
    }
    else
    {
        GC0310_write_cmos_sensor(0x42, (temp_AWB_reg & (~0x02)));
    }
    GC0310_write_cmos_sensor(0xfe, 0x00);

}


/*************************************************************************
* FUNCTION
*   GC0310_GAMMA_Select
*
* DESCRIPTION
*   This function is served for FAE to select the appropriate GAMMA curve.
*
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void GC0310GammaSelect(kal_uint32 GammaLvl)
{
    switch(GammaLvl)
    {
        case GC0310_RGB_Gamma_m1:                       //smallest gamma curve
            GC0310_write_cmos_sensor(0xfe, 0x00);
            GC0310_write_cmos_sensor(0xbf, 0x06);
            GC0310_write_cmos_sensor(0xc0, 0x12);
            GC0310_write_cmos_sensor(0xc1, 0x22);
            GC0310_write_cmos_sensor(0xc2, 0x35);
            GC0310_write_cmos_sensor(0xc3, 0x4b);
            GC0310_write_cmos_sensor(0xc4, 0x5f);
            GC0310_write_cmos_sensor(0xc5, 0x72);
            GC0310_write_cmos_sensor(0xc6, 0x8d);
            GC0310_write_cmos_sensor(0xc7, 0xa4);
            GC0310_write_cmos_sensor(0xc8, 0xb8);
            GC0310_write_cmos_sensor(0xc9, 0xc8);
            GC0310_write_cmos_sensor(0xca, 0xd4);
            GC0310_write_cmos_sensor(0xcb, 0xde);
            GC0310_write_cmos_sensor(0xcc, 0xe6);
            GC0310_write_cmos_sensor(0xcd, 0xf1);
            GC0310_write_cmos_sensor(0xce, 0xf8);
            GC0310_write_cmos_sensor(0xcf, 0xfd);
            break;
        case GC0310_RGB_Gamma_m2:
            GC0310_write_cmos_sensor(0xBF, 0x08);
            GC0310_write_cmos_sensor(0xc0, 0x0F);
            GC0310_write_cmos_sensor(0xc1, 0x21);
            GC0310_write_cmos_sensor(0xc2, 0x32);
            GC0310_write_cmos_sensor(0xc3, 0x43);
            GC0310_write_cmos_sensor(0xc4, 0x50);
            GC0310_write_cmos_sensor(0xc5, 0x5E);
            GC0310_write_cmos_sensor(0xc6, 0x78);
            GC0310_write_cmos_sensor(0xc7, 0x90);
            GC0310_write_cmos_sensor(0xc8, 0xA6);
            GC0310_write_cmos_sensor(0xc9, 0xB9);
            GC0310_write_cmos_sensor(0xcA, 0xC9);
            GC0310_write_cmos_sensor(0xcB, 0xD6);
            GC0310_write_cmos_sensor(0xcC, 0xE0);
            GC0310_write_cmos_sensor(0xcD, 0xEE);
            GC0310_write_cmos_sensor(0xcE, 0xF8);
            GC0310_write_cmos_sensor(0xcF, 0xFF);
            break;

        case GC0310_RGB_Gamma_m3:
            GC0310_write_cmos_sensor(0xbf , 0x0b);
            GC0310_write_cmos_sensor(0xc0 , 0x17);
            GC0310_write_cmos_sensor(0xc1 , 0x2a);
            GC0310_write_cmos_sensor(0xc2 , 0x41);
            GC0310_write_cmos_sensor(0xc3 , 0x54);
            GC0310_write_cmos_sensor(0xc4 , 0x66);
            GC0310_write_cmos_sensor(0xc5 , 0x74);
            GC0310_write_cmos_sensor(0xc6 , 0x8c);
            GC0310_write_cmos_sensor(0xc7 , 0xa3);
            GC0310_write_cmos_sensor(0xc8 , 0xb5);
            GC0310_write_cmos_sensor(0xc9 , 0xc4);
            GC0310_write_cmos_sensor(0xca , 0xd0);
            GC0310_write_cmos_sensor(0xcb , 0xdb);
            GC0310_write_cmos_sensor(0xcc , 0xe5);
            GC0310_write_cmos_sensor(0xcd , 0xf0);
            GC0310_write_cmos_sensor(0xce , 0xf7);
            GC0310_write_cmos_sensor(0xcf , 0xff);
            break;

        case GC0310_RGB_Gamma_m4:
            GC0310_write_cmos_sensor(0xBF, 0x0E);
            GC0310_write_cmos_sensor(0xc0, 0x1C);
            GC0310_write_cmos_sensor(0xc1, 0x34);
            GC0310_write_cmos_sensor(0xc2, 0x48);
            GC0310_write_cmos_sensor(0xc3, 0x5A);
            GC0310_write_cmos_sensor(0xc4, 0x6B);
            GC0310_write_cmos_sensor(0xc5, 0x7B);
            GC0310_write_cmos_sensor(0xc6, 0x95);
            GC0310_write_cmos_sensor(0xc7, 0xAB);
            GC0310_write_cmos_sensor(0xc8, 0xBF);
            GC0310_write_cmos_sensor(0xc9, 0xCE);
            GC0310_write_cmos_sensor(0xcA, 0xD9);
            GC0310_write_cmos_sensor(0xcB, 0xE4);
            GC0310_write_cmos_sensor(0xcC, 0xEC);
            GC0310_write_cmos_sensor(0xcD, 0xF7);
            GC0310_write_cmos_sensor(0xcE, 0xFD);
            GC0310_write_cmos_sensor(0xcF, 0xFF);
            break;

        case GC0310_RGB_Gamma_m5:
            GC0310_write_cmos_sensor(0xBF, 0x10);
            GC0310_write_cmos_sensor(0xc0, 0x20);
            GC0310_write_cmos_sensor(0xc1, 0x38);
            GC0310_write_cmos_sensor(0xc2, 0x4E);
            GC0310_write_cmos_sensor(0xc3, 0x63);
            GC0310_write_cmos_sensor(0xc4, 0x76);
            GC0310_write_cmos_sensor(0xc5, 0x87);
            GC0310_write_cmos_sensor(0xc6, 0xA2);
            GC0310_write_cmos_sensor(0xc7, 0xB8);
            GC0310_write_cmos_sensor(0xc8, 0xCA);
            GC0310_write_cmos_sensor(0xc9, 0xD8);
            GC0310_write_cmos_sensor(0xcA, 0xE3);
            GC0310_write_cmos_sensor(0xcB, 0xEB);
            GC0310_write_cmos_sensor(0xcC, 0xF0);
            GC0310_write_cmos_sensor(0xcD, 0xF8);
            GC0310_write_cmos_sensor(0xcE, 0xFD);
            GC0310_write_cmos_sensor(0xcF, 0xFF);
            break;

        case GC0310_RGB_Gamma_m6:                                       // largest gamma curve
            GC0310_write_cmos_sensor(0xBF, 0x14);
            GC0310_write_cmos_sensor(0xc0, 0x28);
            GC0310_write_cmos_sensor(0xc1, 0x44);
            GC0310_write_cmos_sensor(0xc2, 0x5D);
            GC0310_write_cmos_sensor(0xc3, 0x72);
            GC0310_write_cmos_sensor(0xc4, 0x86);
            GC0310_write_cmos_sensor(0xc5, 0x95);
            GC0310_write_cmos_sensor(0xc6, 0xB1);
            GC0310_write_cmos_sensor(0xc7, 0xC6);
            GC0310_write_cmos_sensor(0xc8, 0xD5);
            GC0310_write_cmos_sensor(0xc9, 0xE1);
            GC0310_write_cmos_sensor(0xcA, 0xEA);
            GC0310_write_cmos_sensor(0xcB, 0xF1);
            GC0310_write_cmos_sensor(0xcC, 0xF5);
            GC0310_write_cmos_sensor(0xcD, 0xFB);
            GC0310_write_cmos_sensor(0xcE, 0xFE);
            GC0310_write_cmos_sensor(0xcF, 0xFF);
            break;
        case GC0310_RGB_Gamma_night:                                    //Gamma for night mode
            GC0310_write_cmos_sensor(0xBF, 0x0B);
            GC0310_write_cmos_sensor(0xc0, 0x16);
            GC0310_write_cmos_sensor(0xc1, 0x29);
            GC0310_write_cmos_sensor(0xc2, 0x3C);
            GC0310_write_cmos_sensor(0xc3, 0x4F);
            GC0310_write_cmos_sensor(0xc4, 0x5F);
            GC0310_write_cmos_sensor(0xc5, 0x6F);
            GC0310_write_cmos_sensor(0xc6, 0x8A);
            GC0310_write_cmos_sensor(0xc7, 0x9F);
            GC0310_write_cmos_sensor(0xc8, 0xB4);
            GC0310_write_cmos_sensor(0xc9, 0xC6);
            GC0310_write_cmos_sensor(0xcA, 0xD3);
            GC0310_write_cmos_sensor(0xcB, 0xDD);
            GC0310_write_cmos_sensor(0xcC, 0xE5);
            GC0310_write_cmos_sensor(0xcD, 0xF1);
            GC0310_write_cmos_sensor(0xcE, 0xFA);
            GC0310_write_cmos_sensor(0xcF, 0xFF);
            break;
        default:
            //GC0310_RGB_Gamma_m1
            GC0310_write_cmos_sensor(0xfe, 0x00);
            GC0310_write_cmos_sensor(0xbf , 0x0b);
            GC0310_write_cmos_sensor(0xc0 , 0x17);
            GC0310_write_cmos_sensor(0xc1 , 0x2a);
            GC0310_write_cmos_sensor(0xc2 , 0x41);
            GC0310_write_cmos_sensor(0xc3 , 0x54);
            GC0310_write_cmos_sensor(0xc4 , 0x66);
            GC0310_write_cmos_sensor(0xc5 , 0x74);
            GC0310_write_cmos_sensor(0xc6 , 0x8c);
            GC0310_write_cmos_sensor(0xc7 , 0xa3);
            GC0310_write_cmos_sensor(0xc8 , 0xb5);
            GC0310_write_cmos_sensor(0xc9 , 0xc4);
            GC0310_write_cmos_sensor(0xca , 0xd0);
            GC0310_write_cmos_sensor(0xcb , 0xdb);
            GC0310_write_cmos_sensor(0xcc , 0xe5);
            GC0310_write_cmos_sensor(0xcd , 0xf0);
            GC0310_write_cmos_sensor(0xce , 0xf7);
            GC0310_write_cmos_sensor(0xcf , 0xff);
            break;
    }
}


/*************************************************************************
 * FUNCTION
 *  GC0310_config_window
 *
 * DESCRIPTION
 *  This function config the hardware window of GC0310 for getting specified
 *  data of that window.
 *
 * PARAMETERS
 *  start_x : start column of the interested window
 *  start_y : start row of the interested window
 *  width  : column widht of the itnerested window
 *  height : row depth of the itnerested window
 *
 * RETURNS
 *  the data that read from GC0310
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC0310_config_window(kal_uint16 startx, kal_uint16 starty, kal_uint16 width, kal_uint16 height)
{
} /* GC0310_config_window */


/*************************************************************************
 * FUNCTION
 *  GC0310_SetGain
 *
 * DESCRIPTION
 *  This function is to set global gain to sensor.
 *
 * PARAMETERS
 *   iGain : sensor global gain(base: 0x40)
 *
 * RETURNS
 *  the actually gain set to sensor.
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint16 GC0310_SetGain(kal_uint16 iGain)
{
    return iGain;
}
/*************************************************************************
* FUNCTION
*   GC0310_Sensor_Init
*
* DESCRIPTION
*   This function apply all of the initial setting to sensor.
*
* PARAMETERS
*   None
*
* RETURNS
*   None
*
*************************************************************************/
void GC0310_Sensor_Init(void)
{
     //VGA setting
/////////////////////////////////////////////////
/////////////////   system reg  /////////////////
/////////////////////////////////////////////////
CamWriteCmosSensor(0xfe,0xf0);
CamWriteCmosSensor(0xfe,0xf0);
CamWriteCmosSensor(0xfe,0x00);

CamWriteCmosSensor(0xfc,0x16); //4e
CamWriteCmosSensor(0xfc,0x16); //4e // [0]apwd [6]regf_clk_gate
CamWriteCmosSensor(0xf2,0x07); //sync output
CamWriteCmosSensor(0xf3,0x83); //ff//1f//01 data output
CamWriteCmosSensor(0xf5,0x07); //sck_dely
CamWriteCmosSensor(0xf7,0x88); //f8//fd
CamWriteCmosSensor(0xf8,0x00);
CamWriteCmosSensor(0xf9,0x4f); //0f//01
CamWriteCmosSensor(0xfa,0x10); //10: 2-lane, 32:1-lane ,ic.hsu
CamWriteCmosSensor(0xfc,0xce);
CamWriteCmosSensor(0xfd,0x00);

/////////////////////////////////////////////////
	/////////////////   CISCTL reg  /////////////////
/////////////////////////////////////////////////
CamWriteCmosSensor(0x00,0x2f); //2f//0f//02//01gA1U2DOX
CamWriteCmosSensor(0x01,0x0f); //06
CamWriteCmosSensor(0x02,0x04);
#if 1
CamWriteCmosSensor(0x03,0x01);
CamWriteCmosSensor(0x04,0x89);
CamWriteCmosSensor(0x05,0x00);
CamWriteCmosSensor(0x06,0x6a); //HB
CamWriteCmosSensor(0x07,0x00);
CamWriteCmosSensor(0x08,0x0A); //VB

#else

CamWriteCmosSensor(0x03,0x01);
CamWriteCmosSensor(0x04,0xea);
CamWriteCmosSensor(0x05,0x00);
CamWriteCmosSensor(0x06,0xa3); //HB
CamWriteCmosSensor(0x07,0x00);
CamWriteCmosSensor(0x08,0x12); //VB
#endif
CamWriteCmosSensor(0x09,0x00); //row start
CamWriteCmosSensor(0x0a,0x00); //
CamWriteCmosSensor(0x0b,0x00); //col start
CamWriteCmosSensor(0x0c,0x06);
CamWriteCmosSensor(0x0d,0x01); //height
CamWriteCmosSensor(0x0e,0xe8); //height
CamWriteCmosSensor(0x0f,0x02); //width
CamWriteCmosSensor(0x10,0x88); //height
CamWriteCmosSensor(0x11,0x10); //0x10 : 66.62ms, 0x11 : 66.75ms
CamWriteCmosSensor(0x12,0x02); //ST
CamWriteCmosSensor(0x13,0x02); //ET
CamWriteCmosSensor(0x17,0x14);
CamWriteCmosSensor(0x18,0x0a); //0a//[4]double reset
CamWriteCmosSensor(0x19,0x14); //AD pipeline
CamWriteCmosSensor(0x1b,0x48);
CamWriteCmosSensor(0x1e,0x6b); //3b//col bias
CamWriteCmosSensor(0x1f,0x28); //20//00//08//txlow
CamWriteCmosSensor(0x20,0x89); //88//0c//[3:2]DA15
CamWriteCmosSensor(0x21,0x49); //48//[3] txhigh
CamWriteCmosSensor(0x22,0xb0);
CamWriteCmosSensor(0x23,0x04); //[1:0]vcm_r
CamWriteCmosSensor(0x24,0xff); //Cy?AUA|//15
CamWriteCmosSensor(0x34,0x20); //[6:4] rsg high//Oo?Orange

/////////////////////////////////////////////////
////////////////////   BLK   ////////////////////
/////////////////////////////////////////////////
CamWriteCmosSensor(0x26,0x23); //[1]dark_current_en [0]offset_en
CamWriteCmosSensor(0x28,0xff); //BLK_limie_value
CamWriteCmosSensor(0x29,0x00); //global offset
CamWriteCmosSensor(0x33,0x18); //offset_ratio
CamWriteCmosSensor(0x37,0x20); //dark_current_ratio
CamWriteCmosSensor(0x47,0x80); //a7
CamWriteCmosSensor(0x4e,0x66); //select_row
CamWriteCmosSensor(0xa8,0x02); //win_width_dark, same with crop_win_width
CamWriteCmosSensor(0xa9,0x80);

/////////////////////////////////////////////////
//////////////////   ISP reg  ///////////////////
/////////////////////////////////////////////////
CamWriteCmosSensor(0x40,0x7f);//48
CamWriteCmosSensor(0x41,0x21);//00//[0]curve_en
CamWriteCmosSensor(0x42,0xcf);  //0a//[1]awn_en
CamWriteCmosSensor(0x44,0x02);
CamWriteCmosSensor(0x46,0x02); //03//sync
CamWriteCmosSensor(0x4a,0x11);
CamWriteCmosSensor(0x4b,0x01);
CamWriteCmosSensor(0x4c,0x20);//00[5]pretect exp
CamWriteCmosSensor(0x4d,0x05);//update gain mode
CamWriteCmosSensor(0x4f,0x01);
CamWriteCmosSensor(0x50,0x01);//crop enable
CamWriteCmosSensor(0x55,0x01);//crop window height
CamWriteCmosSensor(0x56,0xe0);
CamWriteCmosSensor(0x57,0x02);//crop window width
CamWriteCmosSensor(0x58,0x80);

/////////////////////////////////////////////////
///////////////////   GAIN   ////////////////////
/////////////////////////////////////////////////
CamWriteCmosSensor(0x70,0x50); //70 //80//global gain
CamWriteCmosSensor(0x5a,0x98);//84//analog gain 0
CamWriteCmosSensor(0x5b,0xdc);//c9
CamWriteCmosSensor(0x5c,0xfe);//ed//not use pga gain highest level

//CamWriteCmosSensor(0x77,0x74);//awb gain
//CamWriteCmosSensor(0x78,0x40);
//CamWriteCmosSensor(0x79,0x5f);

CamWriteCmosSensor(0x7a,0x80); //R ratio/////////////////////////////////////////////////
///////////////////   DNDD  /////////////////////
/////////////////////////////////////////////////
CamWriteCmosSensor(0x82,0x05); //05
CamWriteCmosSensor(0x83,0x0b);

/////////////////////////////////////////////////
	//////////////////   EEINTP  ////////////////////
/////////////////////////////////////////////////
CamWriteCmosSensor(0x8f,0xff);//skin th
CamWriteCmosSensor(0x90,0x8c);
CamWriteCmosSensor(0x92,0x08);//10//05
CamWriteCmosSensor(0x93,0x03);//10//00
CamWriteCmosSensor(0x95,0x53);//88
CamWriteCmosSensor(0x96,0x56);//88

/////////////////////////////////////////////////
	/////////////////////  ASDE  ////////////////////
/////////////////////////////////////////////////
CamWriteCmosSensor(0xfe,0x00);

CamWriteCmosSensor(0x9a,0x20); //Y_limit
CamWriteCmosSensor(0x9b,0xb0); //Y_slope
CamWriteCmosSensor(0x9c,0x40); //LSC_th
CamWriteCmosSensor(0x9d,0xa0); //80//LSC_slope

CamWriteCmosSensor(0xa1,0xf0);//00
CamWriteCmosSensor(0xa2,0x32); //12
CamWriteCmosSensor(0xa4,0x30); //Sat_slope
CamWriteCmosSensor(0xa5,0x30); //Sat_limit
CamWriteCmosSensor(0xaa,0x50); //OT_th
CamWriteCmosSensor(0xac,0x22); //DN_slope_low

/////////////////////////////////////////////////
	///////////////////   YCP  //////////////////////
/////////////////////////////////////////////////
CamWriteCmosSensor(0xd0,0x40);
CamWriteCmosSensor(0xd1,0x2b);//28//38
CamWriteCmosSensor(0xd2,0x2b);//28//38
CamWriteCmosSensor(0xd3,0x40);//3c
CamWriteCmosSensor(0xd6,0xf2);
CamWriteCmosSensor(0xd7,0x1b);
CamWriteCmosSensor(0xd8,0x18);
CamWriteCmosSensor(0xdd,0x73);//no edge dec saturation

/////////////////////////////////////////////////
	////////////////////   AEC   ////////////////////
/////////////////////////////////////////////////
CamWriteCmosSensor(0xfe,0x01);
CamWriteCmosSensor(0x05,0x30); //40//AEC_center-x1 X16
CamWriteCmosSensor(0x06,0x75); //70//X2
CamWriteCmosSensor(0x07,0x40); //60//Y1 X8
CamWriteCmosSensor(0x08,0xb0); //90//Y2
CamWriteCmosSensor(0x0a,0xc5);//c1//81//81//01//f
CamWriteCmosSensor(0x12,0x52);
CamWriteCmosSensor(0x13,0x35); //28 //78//Y target

CamWriteCmosSensor(0x1f,0x30); //QQ
CamWriteCmosSensor(0x20,0xc0); //QQ 80
GC0310_write_cmos_sensor(0x25,0x00); //step
GC0310_write_cmos_sensor(0x26,0x96);

CamWriteCmosSensor(0x27,0x01); //30fps
CamWriteCmosSensor(0x28,0xc2);
CamWriteCmosSensor(0x29,0x02); //25fps
CamWriteCmosSensor(0x2a,0x58);
CamWriteCmosSensor(0x2b,0x02); //20fps
CamWriteCmosSensor(0x2c,0xee);
CamWriteCmosSensor(0x2d,0x04); //15fps
CamWriteCmosSensor(0x2e,0x1a);
CamWriteCmosSensor(0x3c,0x00);
CamWriteCmosSensor(0x3e,0x40); //40
CamWriteCmosSensor(0x3f,0x5c); //57
CamWriteCmosSensor(0x40,0x7b); //7d
CamWriteCmosSensor(0x41,0xbd); //a7
CamWriteCmosSensor(0x42,0xf6); //01
CamWriteCmosSensor(0x43,0x63); //4e
//CamWriteCmosSensor(0x04,0xe3);
CamWriteCmosSensor(0x03,0x60); //70

CamWriteCmosSensor(0x44,0x03);//04
/////////////////////////////////////////////////
	////////////////////   AWB   ////////////////////
/////////////////////////////////////////////////

CamWriteCmosSensor(0x1c,0x91);//21 //luma_level_for_awb_select

CamWriteCmosSensor(0x61,0x9e);
CamWriteCmosSensor(0x62,0xa5);
CamWriteCmosSensor(0x63,0xd0);//a0//d0//a0//a0 x1-th
CamWriteCmosSensor(0x64,0x20);
CamWriteCmosSensor(0x65,0x06);//60
CamWriteCmosSensor(0x66,0x02);//02//04
CamWriteCmosSensor(0x67,0x84);//82//82
CamWriteCmosSensor(0x6b,0x20);//40//00//40
CamWriteCmosSensor(0x6c,0x10);//00
CamWriteCmosSensor(0x6d,0x32);//00
CamWriteCmosSensor(0x6e,0x38);//f8//38//indoor mode
CamWriteCmosSensor(0x6f,0x62);//80//a0//indoor-th
CamWriteCmosSensor(0x70,0x00);
CamWriteCmosSensor(0x78,0xb0);

CamWriteCmosSensor(0xfe,0x01);// gain limit
CamWriteCmosSensor(0x0c,0x01);

CamWriteCmosSensor(0xfe,0x01);// G gain limit
CamWriteCmosSensor(0x78,0xa0);

CamWriteCmosSensor(0xfe,0x01);
CamWriteCmosSensor(0x1c,0x21);

CamWriteCmosSensor(0xfe,0x01);
CamWriteCmosSensor(0x90,0x00);
CamWriteCmosSensor(0x91,0x00);
CamWriteCmosSensor(0x92,0xf0);
CamWriteCmosSensor(0x93,0xd1);
CamWriteCmosSensor(0x94,0x50);
CamWriteCmosSensor(0x95,0xfd);
CamWriteCmosSensor(0x96,0xdf);
CamWriteCmosSensor(0x97,0x1f);
CamWriteCmosSensor(0x98,0x00);
CamWriteCmosSensor(0x99,0xa5);
CamWriteCmosSensor(0x9a,0x23);
CamWriteCmosSensor(0x9b,0x0c);
CamWriteCmosSensor(0x9c,0x55);
CamWriteCmosSensor(0x9d,0x3b);
CamWriteCmosSensor(0x9e,0xaa);
CamWriteCmosSensor(0x9f,0x00);
CamWriteCmosSensor(0xa0,0x00);
CamWriteCmosSensor(0xa1,0x00);
CamWriteCmosSensor(0xa2,0x00);
CamWriteCmosSensor(0xa3,0x00);
CamWriteCmosSensor(0xa4,0x00);
CamWriteCmosSensor(0xa5,0x00);
CamWriteCmosSensor(0xa6,0xcc);
CamWriteCmosSensor(0xa7,0xb7);
CamWriteCmosSensor(0xa8,0x50);
CamWriteCmosSensor(0xa9,0xb6);
CamWriteCmosSensor(0xaa,0x8d);
CamWriteCmosSensor(0xab,0xa8);
CamWriteCmosSensor(0xac,0x85);
CamWriteCmosSensor(0xad,0x55);
CamWriteCmosSensor(0xae,0xbb);
CamWriteCmosSensor(0xaf,0xa9);
CamWriteCmosSensor(0xb0,0xba);
CamWriteCmosSensor(0xb1,0xa2);
CamWriteCmosSensor(0xb2,0x55);
CamWriteCmosSensor(0xb3,0x00);
CamWriteCmosSensor(0xb4,0x00);
CamWriteCmosSensor(0xb5,0x00);
CamWriteCmosSensor(0xb6,0x00);
CamWriteCmosSensor(0xb7,0x00);
CamWriteCmosSensor(0xb8,0x9e);
CamWriteCmosSensor(0xb9,0xc9);

/////////////////////////////////////////////////
	////////////////////   CC    ////////////////////
/////////////////////////////////////////////////
CamWriteCmosSensor(0xfe,0x01);
CamWriteCmosSensor(0xd0,0x44);
CamWriteCmosSensor(0xd1,0xFb);
CamWriteCmosSensor(0xd2,0x00);
CamWriteCmosSensor(0xd3,0x08);
CamWriteCmosSensor(0xd4,0x40);
CamWriteCmosSensor(0xd5,0x0d);
CamWriteCmosSensor(0xfe,0x00);
/////////////////////////////////////////////////
	////////////////////   LSC   ////////////////////
/////////////////////////////////////////////////
CamWriteCmosSensor(0xfe,0x01);
CamWriteCmosSensor(0x76,0x60);//R_gain_limit
CamWriteCmosSensor(0xc1,0x3c);//row center
CamWriteCmosSensor(0xc2,0x50);//col center
CamWriteCmosSensor(0xc3,0x00);//b4 sign
CamWriteCmosSensor(0xc4,0x4b);// b2 R
CamWriteCmosSensor(0xc5,0x38);//G
CamWriteCmosSensor(0xc6,0x38);//B
CamWriteCmosSensor(0xc7,0x10);//b4 R
CamWriteCmosSensor(0xc8,0x00);//G
CamWriteCmosSensor(0xc9,0x00);//b
CamWriteCmosSensor(0xdc,0x20);//lsc_Y_dark_th
CamWriteCmosSensor(0xdd,0x10);//lsc_Y_dark_slope
CamWriteCmosSensor(0xdf,0x00);
CamWriteCmosSensor(0xde,0x00);

/////////////////////////////////////////////////
	///////////////////  Histogram  /////////////////
/////////////////////////////////////////////////
CamWriteCmosSensor(0x01,0x10); //precision
CamWriteCmosSensor(0x0b,0x31); //close fix_target_mode
CamWriteCmosSensor(0x0e,0x6c); //th_low
CamWriteCmosSensor(0x0f,0x0f); //color_diff_th
CamWriteCmosSensor(0x10,0x6e); //th_high
CamWriteCmosSensor(0x12,0xa0); //a1//enable
CamWriteCmosSensor(0x15,0x40); //target_Y_limit
CamWriteCmosSensor(0x16,0x60); //th_for_disable_hist
CamWriteCmosSensor(0x17,0x20); //luma_slope

/////////////////////////////////////////////////
	//////////////   Measure Window   ///////////////
/////////////////////////////////////////////////
CamWriteCmosSensor(0xcc,0x0c);//aec window size
CamWriteCmosSensor(0xcd,0x10);
CamWriteCmosSensor(0xce,0xa0);
CamWriteCmosSensor(0xcf,0xe6);

/////////////////////////////////////////////////
	/////////////////   dark sun   //////////////////
/////////////////////////////////////////////////
CamWriteCmosSensor(0x45,0xf7);
CamWriteCmosSensor(0x46,0xff); //f0//sun vaule th
CamWriteCmosSensor(0x47,0x15);
CamWriteCmosSensor(0x48,0x03); //sun mode
CamWriteCmosSensor(0x4f,0x60); //sun_clamp

/////////////////////////////////////////////////
	/////////////////////  SPI   ////////////////////
/////////////////////////////////////////////////
CamWriteCmosSensor(0xfe,0x03);
CamWriteCmosSensor(0x01,0x00);
CamWriteCmosSensor(0x02,0x00);
CamWriteCmosSensor(0x10,0x00);
CamWriteCmosSensor(0x15,0x00);
CamWriteCmosSensor(0x17,0x00); //01:2-lqne, 03:1-lane, ic.hsu
CamWriteCmosSensor(0x04,0x01);
CamWriteCmosSensor(0x05,0x00);
CamWriteCmosSensor(0x40,0x00);
CamWriteCmosSensor(0x51,0x03);
CamWriteCmosSensor(0x52,0x20); //a2 //a0//80//00
CamWriteCmosSensor(0x53,0xa4); //24
CamWriteCmosSensor(0x54,0x20);
CamWriteCmosSensor(0x55,0x20); //QQ//01
CamWriteCmosSensor(0x5a,0x40); //00 //yuv
CamWriteCmosSensor(0x5b,0x80);
CamWriteCmosSensor(0x5c,0x02);
CamWriteCmosSensor(0x5d,0xe0);
CamWriteCmosSensor(0x5e,0x01);

write_cmos_sensor2(0x64,GC0310_read_cmos_sensor(0x64)|0x02); 	// always clock
SENSORDB("always csk 0x64= 0x%04x\n", GC0310_read_cmos_sensor(0x64));

CamWriteCmosSensor(0xfe,0x00);
Sleep(60);//delay 2 frame for stable data for calibration

}

UINT32 GC0310GetSensorID(UINT32 *sensorID)
{
    int  retry = 3;
    // check if sensor ID correct
    do {
        *sensorID=((GC0310_read_cmos_sensor(0xf0)<< 8)|GC0310_read_cmos_sensor(0xf1));
        if (*sensorID == GC0310_SENSOR_ID)
            break;
        LOG_INF("Read Sensor ID Fail = 0x%04x\n", *sensorID);
        retry--;
    } while (retry > 0);

    if (*sensorID != GC0310_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF;
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    LOG_INF("GC0310 Sensor Read ID(0x%x)\r\n", *sensorID);
    return ERROR_NONE;
}




/*************************************************************************
* FUNCTION
*   GC0310_Write_More_Registers
*
* DESCRIPTION
*   This function is served for FAE to modify the necessary Init Regs. Do not modify the regs
*     in init_GC0310() directly.
*
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void GC0310_Write_More_Registers(void)
{

}

/*************************************************************************
 * FUNCTION
 *  GC0310Open
 *
 * DESCRIPTION
 *  This function initialize the registers of CMOS sensor
 *
 * PARAMETERS
 *  None
 *
 * RETURNS
 *  None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
UINT32 GC0310Open(void)
{
    volatile signed char i;
    kal_uint16 sensor_id=0;

    LOG_INF("<Jet> Entry GC0310Open!!!\r\n");

    Sleep(10);

    //Read sensor ID to adjust I2C is OK?
    for(i=0;i<3;i++)
    {
        sensor_id = ((GC0310_read_cmos_sensor(0xf0) << 8) | GC0310_read_cmos_sensor(0xf1));
        if(sensor_id != GC0310_SENSOR_ID)
        {
            LOG_INF("GC0310mipi Read Sensor ID Fail[open] = 0x%x\n", sensor_id);
            return ERROR_SENSOR_CONNECT_FAIL;
        }
    }
    Sleep(10);

    LOG_INF("GC0310 Sensor Read ID(0x%x)\r\n",sensor_id);
    GC0310_Sensor_Init();
    //GC0310_Write_More_Registers();

    spin_lock(&GC0310_drv_lock);
    GC0310_night_mode_enable = KAL_FALSE;
    GC0310_MPEG4_encode_mode = KAL_FALSE;
    spin_unlock(&GC0310_drv_lock);

    return ERROR_NONE;
} /* GC0310Open */


/*************************************************************************
 * FUNCTION
 *  GC0310Close
 *
 * DESCRIPTION
 *  This function is to turn off sensor module power.
 *
 * PARAMETERS
 *  None
 *
 * RETURNS
 *  None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
UINT32 GC0310Close(void)
{
    return ERROR_NONE;
} /* GC0310Close */


/*************************************************************************
 * FUNCTION
 * GC0310Preview
 *
 * DESCRIPTION
 *  This function start the sensor preview.
 *
 * PARAMETERS
 *  *image_window : address pointer of pixel numbers in one period of HSYNC
 *  *sensor_config_data : address pointer of line numbers in one period of VSYNC
 *
 * RETURNS
 *  None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
UINT32 GC0310Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
    kal_uint32 iTemp;
    kal_uint16 iStartX = 0, iStartY = 1;

    LOG_INF("Enter GC0310Preview function!!!\r\n");

    GC0310StreamOn();

    image_window->GrabStartX= IMAGE_SENSOR_VGA_GRAB_PIXELS;
    image_window->GrabStartY= IMAGE_SENSOR_VGA_GRAB_LINES;
    image_window->ExposureWindowWidth = IMAGE_SENSOR_PV_WIDTH;
    image_window->ExposureWindowHeight =IMAGE_SENSOR_PV_HEIGHT;

    //GC0310_Set_Mirrorflip(IMAGE_HV_MIRROR);

    // copy sensor_config_data
    memcpy(&GC0310SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

//    GC0310NightMode(GC0310_night_mode_enable);
    return ERROR_NONE;
} /* GC0310Preview */


/*************************************************************************
 * FUNCTION
 *  GC0310Capture
 *
 * DESCRIPTION
 *  This function setup the CMOS sensor in capture MY_OUTPUT mode
 *
 * PARAMETERS
 *
 * RETURNS
 *  None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
UINT32 GC0310Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
    image_window->GrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
    image_window->GrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
    image_window->ExposureWindowWidth= IMAGE_SENSOR_FULL_WIDTH;
    image_window->ExposureWindowHeight = IMAGE_SENSOR_FULL_HEIGHT;

    // copy sensor_config_data
    memcpy(&GC0310SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    GC0310StreamOn();
    return ERROR_NONE;
} /* GC0310_Capture() */

UINT32 GC0310Video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{

    image_window->GrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
    image_window->GrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
    image_window->ExposureWindowWidth= IMAGE_SENSOR_FULL_WIDTH;
    image_window->ExposureWindowHeight = IMAGE_SENSOR_FULL_HEIGHT;

    // copy sensor_config_data
    memcpy(&GC0310SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    GC0310StreamOn();
    return ERROR_NONE;
} /* GC0310_Capture() */



UINT32 GC0310GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    pSensorResolution->SensorFullWidth=IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight=IMAGE_SENSOR_FULL_HEIGHT;
    pSensorResolution->SensorPreviewWidth=IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight=IMAGE_SENSOR_PV_HEIGHT;
    pSensorResolution->SensorVideoWidth=IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorVideoHeight=IMAGE_SENSOR_PV_HEIGHT;

    pSensorResolution->SensorHighSpeedVideoWidth=IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorHighSpeedVideoHeight=IMAGE_SENSOR_PV_HEIGHT;

    pSensorResolution->SensorSlimVideoWidth=IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorSlimVideoHeight=IMAGE_SENSOR_PV_HEIGHT;
    return ERROR_NONE;
} /* GC0310GetResolution() */


UINT32 GC0310GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
        MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    pSensorInfo->SensorPreviewResolutionX=IMAGE_SENSOR_PV_WIDTH;
    pSensorInfo->SensorPreviewResolutionY=IMAGE_SENSOR_PV_HEIGHT;
    pSensorInfo->SensorFullResolutionX=IMAGE_SENSOR_FULL_WIDTH;
    pSensorInfo->SensorFullResolutionY=IMAGE_SENSOR_FULL_HEIGHT;

    pSensorInfo->SensorCameraPreviewFrameRate=30;
    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=10;
    pSensorInfo->SensorWebCamCaptureFrameRate=15;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=1;
//  pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_VYUY;
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV;

    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 1;
    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_SERIAL;//MIPI setting
    pSensorInfo->CaptureDelayFrame = 2;
    pSensorInfo->PreviewDelayFrame = 2;
    pSensorInfo->VideoDelayFrame = 4;

    pSensorInfo->SensorMasterClockSwitch = 0;
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_6MA;

    pSensorInfo->HighSpeedVideoDelayFrame = 4;
    pSensorInfo->SlimVideoDelayFrame = 4;
    pSensorInfo->SensorModeNum = 5;

    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    default:
        pSensorInfo->SensorClockFreq=48;
        pSensorInfo->SensorClockDividCount= 3;
        pSensorInfo->SensorClockRisingCount=0;
        pSensorInfo->SensorClockFallingCount=2;
        pSensorInfo->SensorPixelClockCount=3;
        pSensorInfo->SensorDataLatchCount=2;
        pSensorInfo->SensorGrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
        pSensorInfo->SensorGrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
        //MIPI setting
	    pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;
        pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
        pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
        pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x
        pSensorInfo->SensorPacketECCOrder = 1;
        /*SCAM setting*/
        pSensorInfo->SCAM_DataNumber = SCAM_2_DATA_CHANNEL;
        pSensorInfo->SCAM_DDR_En = 1;
        pSensorInfo->SCAM_CLK_INV = 0;
		pSensorInfo->SCAM_DEFAULT_DELAY = 64;//0~31 negtive delay, 32~63 postive delay,should be 64 to keep always auto calibration
		pSensorInfo->SCAM_CRC_En = 1;
		pSensorInfo->SCAM_SOF_src = 1;
		pSensorInfo->SCAM_Timout_Cali=0x34BC0;// 34BC0//30fps
                                       // 0x3D9DEA//14.5fps
        break;
    }
    GC0310PixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &GC0310SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* GC0310GetInfo() */


UINT32 GC0310Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{

    LOG_INF("Entry GC0310Control, ScenarioId = %d!!!\r\n", ScenarioId);
//return	ERROR_NONE;
	switch (ScenarioId)
	{

		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			GC0310Preview(pImageWindow, pSensorConfigData);
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			GC0310Capture(pImageWindow, pSensorConfigData);
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			GC0310Video(pImageWindow, pSensorConfigData);
			break;

		default:
			LOG_INF("Error ScenarioId setting");
			GC0310Preview(pImageWindow, pSensorConfigData);
			return ERROR_INVALID_SCENARIO_ID;
	}
    return ERROR_NONE;
}   /* GC0310Control() */

BOOL GC0310_set_param_wb(UINT16 para)
{
    LOG_INF("GC0310_set_param_wb para = %d\n", para);
    spin_lock(&GC0310_drv_lock);
    GC0310_CurStatus_AWB = para;
    spin_unlock(&GC0310_drv_lock);

    GC0310_write_cmos_sensor(0xfe, 0x00);
    switch (para)
    {
        case AWB_MODE_OFF:
            GC0310_awb_enable(KAL_FALSE);
            GC0310_write_cmos_sensor(0x77,0x74);
            GC0310_write_cmos_sensor(0x78,0x40);
            GC0310_write_cmos_sensor(0x79,0x5f);
        break;

        case AWB_MODE_AUTO:
            GC0310_write_cmos_sensor(0x77,0x74);
            GC0310_write_cmos_sensor(0x78,0x40);
            GC0310_write_cmos_sensor(0x79,0x5f);
            GC0310_awb_enable(KAL_TRUE);
        break;

        case AWB_MODE_CLOUDY_DAYLIGHT: //cloudy
            GC0310_awb_enable(KAL_FALSE);
            GC0310_write_cmos_sensor(0x77, 0x8c);
            GC0310_write_cmos_sensor(0x78, 0x50);
            GC0310_write_cmos_sensor(0x79, 0x40);
        break;

        case AWB_MODE_DAYLIGHT: //sunny
            GC0310_awb_enable(KAL_FALSE);
            GC0310_write_cmos_sensor(0x77, 0x74);
            GC0310_write_cmos_sensor(0x78, 0x52);
            GC0310_write_cmos_sensor(0x79, 0x40);
        break;

        case AWB_MODE_INCANDESCENT: //office
            GC0310_awb_enable(KAL_FALSE);
            GC0310_write_cmos_sensor(0x77, 0x48);
            GC0310_write_cmos_sensor(0x78, 0x40);
            GC0310_write_cmos_sensor(0x79, 0x5c);
        break;

        case AWB_MODE_TUNGSTEN: //home
            GC0310_awb_enable(KAL_FALSE);
            GC0310_write_cmos_sensor(0x77, 0x40); //WB_manual_gain
            GC0310_write_cmos_sensor(0x78, 0x54);
            GC0310_write_cmos_sensor(0x79, 0x70);
        break;

        case AWB_MODE_FLUORESCENT:
            GC0310_awb_enable(KAL_FALSE);
            GC0310_write_cmos_sensor(0x77, 0x40);
            GC0310_write_cmos_sensor(0x78, 0x42);
            GC0310_write_cmos_sensor(0x79, 0x50);
        break;

        default:
        return FALSE;
    }
    GC0310_write_cmos_sensor(0xfe, 0x00);

    return TRUE;
} /* GC0310_set_param_wb */


BOOL GC0310_set_param_effect(UINT16 para)
{
    kal_uint32  ret = KAL_TRUE;

    switch (para)
    {
        case MEFFECT_OFF:
            GC0310_write_cmos_sensor(0x43 , 0x00);
        break;

        case MEFFECT_SEPIA:
            GC0310_write_cmos_sensor(0x43 , 0x00);
	    Sleep(50);
            GC0310_write_cmos_sensor(0xda , 0xd0);
            GC0310_write_cmos_sensor(0xdb , 0x28);
            GC0310_write_cmos_sensor(0x43 , 0x02);
        break;

        case MEFFECT_NEGATIVE:
            GC0310_write_cmos_sensor(0x43 , 0x01);
        break;

        case MEFFECT_SEPIAGREEN:
            GC0310_write_cmos_sensor(0x43 , 0x00);
            Sleep(50);
            GC0310_write_cmos_sensor(0xda , 0xc0);
            GC0310_write_cmos_sensor(0xdb , 0xc0);
            GC0310_write_cmos_sensor(0x43 , 0x02);
        break;

        case MEFFECT_SEPIABLUE:
            GC0310_write_cmos_sensor(0x43 , 0x00);
            Sleep(50);
            GC0310_write_cmos_sensor(0xda , 0x28);
            GC0310_write_cmos_sensor(0xdb , 0xa0);
            GC0310_write_cmos_sensor(0x43 , 0x02);
        break;

        case MEFFECT_MONO:
            GC0310_write_cmos_sensor(0x43 , 0x00);
            Sleep(50);
            GC0310_write_cmos_sensor(0xda , 0x00);
            GC0310_write_cmos_sensor(0xdb , 0x00);
            GC0310_write_cmos_sensor(0x43 , 0x02);
        break;
        default:
            ret = FALSE;
    }

    return ret;

} /* GC0310_set_param_effect */


BOOL GC0310_set_param_banding(UINT16 para)
{

    LOG_INF("Enter GC0310_set_param_banding!, para = %d\r\n", para);
    switch (para)
    {
        case AE_FLICKER_MODE_AUTO:
        case AE_FLICKER_MODE_OFF:
        case AE_FLICKER_MODE_50HZ:
            CamWriteCmosSensor(0xfe,0x00);
            CamWriteCmosSensor(0x05,0x00);
            CamWriteCmosSensor(0x06,0x6a);
            CamWriteCmosSensor(0x07,0x00);
            CamWriteCmosSensor(0x08,0x0a);

            CamWriteCmosSensor(0xfe,0x01);
            CamWriteCmosSensor(0x25,0x00); //step
            CamWriteCmosSensor(0x26,0x96);

            CamWriteCmosSensor(0x27,0x01); //30fps
            CamWriteCmosSensor(0x28,0xc2);
            CamWriteCmosSensor(0x29,0x02); //20fps
            CamWriteCmosSensor(0x2a,0xee);
            CamWriteCmosSensor(0x2b,0x05); //10fps
            CamWriteCmosSensor(0x2c,0xdc);
            CamWriteCmosSensor(0x2d,0x05); //10fps
            CamWriteCmosSensor(0x2e,0xdc);
            CamWriteCmosSensor(0x3c,0x30);
            CamWriteCmosSensor(0xfe,0x00);
            GC0310_banding_state = GC0310_BANDING_50HZ;  // 0~50hz; 1~60hz
            break;

        case AE_FLICKER_MODE_60HZ:
            CamWriteCmosSensor(0xfe,0x00);
            CamWriteCmosSensor(0x05,0x00);
            CamWriteCmosSensor(0x06,0x6a);
            CamWriteCmosSensor(0x07,0x00);
            CamWriteCmosSensor(0x08,0x0a);

            CamWriteCmosSensor(0xfe,0x01);
            CamWriteCmosSensor(0x25,0x00); //step
            CamWriteCmosSensor(0x26,0x7d);

            CamWriteCmosSensor(0x27,0x01); //30fps
            CamWriteCmosSensor(0x28,0x77);
            CamWriteCmosSensor(0x29,0x02); //20fps
            CamWriteCmosSensor(0x2a,0xee);
            CamWriteCmosSensor(0x2b,0x05); //10fps
            CamWriteCmosSensor(0x2c,0xdc);
            CamWriteCmosSensor(0x2d,0x05); //10fps
            CamWriteCmosSensor(0x2e,0xdc);
            CamWriteCmosSensor(0x3c,0x30);
            CamWriteCmosSensor(0xfe,0x00);
            GC0310_banding_state = GC0310_BANDING_60HZ; // 0~50hz; 1~60hz
        break;
        default:
        return FALSE;
    }
    GC0310_write_cmos_sensor(0xfe, 0x00);
    LOG_INF("shutter %d\n",GC0310_Read_Shutter());

    return TRUE;
} /* GC0310_set_param_banding */

BOOL GC0310_set_param_exposure(UINT16 para)
{

    switch (para)
    {

        case AE_EV_COMP_n20:
            GC0310_write_cmos_sensor(0xfe, 0x01);
            GC0310_write_cmos_sensor(0x13, 0x20);
            GC0310_write_cmos_sensor(0xfe, 0x00);
        break;

        case AE_EV_COMP_n10:
            GC0310_write_cmos_sensor(0xfe, 0x01);
            GC0310_write_cmos_sensor(0x13, 0x28);  // 28 to 10
            GC0310_write_cmos_sensor(0xfe, 0x00);
        break;

        case AE_EV_COMP_00:
            GC0310_write_cmos_sensor(0xfe, 0x01);
            GC0310_write_cmos_sensor(0x13, 0x38);//35
            GC0310_write_cmos_sensor(0xfe, 0x00);
        break;

        case AE_EV_COMP_15:
        case AE_EV_COMP_10:
            GC0310_write_cmos_sensor(0xfe, 0x01);
            GC0310_write_cmos_sensor(0x13, 0x48);  // 48 to 60
            GC0310_write_cmos_sensor(0xfe, 0x00);
        break;

        case AE_EV_COMP_20:
            GC0310_write_cmos_sensor(0xfe, 0x01);
            GC0310_write_cmos_sensor(0x13, 0x50);
            GC0310_write_cmos_sensor(0xfe, 0x00);
        break;
        default:
        return FALSE;
    }

    return TRUE;
} /* GC0310_set_param_exposure */

void set_fixframerate_by_banding(UINT16 u2FrameRate,  kal_bool bBanding)
{
    if ( 30 == u2FrameRate){
        	           LOG_INF("fix 30 fps");

		if ( GC0310_BANDING_50HZ == bBanding )
		{
			CamWriteCmosSensor(0xfe,0x00);
			CamWriteCmosSensor(0x05,0x00);
			CamWriteCmosSensor(0x06,0x6a);
			CamWriteCmosSensor(0x07,0x00);
			CamWriteCmosSensor(0x08,0x0a);

			CamWriteCmosSensor(0xfe,0x01);
			CamWriteCmosSensor(0x25,0x00); //step
			CamWriteCmosSensor(0x26,0x96);

			CamWriteCmosSensor(0x27,0x01); //30fps
			CamWriteCmosSensor(0x28,0xc2);
			CamWriteCmosSensor(0x29,0x02); //20fps
			CamWriteCmosSensor(0x2a,0xee);
			CamWriteCmosSensor(0x2b,0x05); //10fps
			CamWriteCmosSensor(0x2c,0xdc);
			CamWriteCmosSensor(0x2d,0x0b); //5fps
			CamWriteCmosSensor(0x2e,0xb8);
			CamWriteCmosSensor(0x3c,0x00);
			CamWriteCmosSensor(0xfe,0x00);

		}
		else if ( GC0310_BANDING_60HZ == bBanding  )
		{
			CamWriteCmosSensor(0xfe,0x00);
			CamWriteCmosSensor(0x05,0x00);
			CamWriteCmosSensor(0x06,0x6a);
			CamWriteCmosSensor(0x07,0x00);
			CamWriteCmosSensor(0x08,0x0a);

			CamWriteCmosSensor(0xfe,0x01);
			CamWriteCmosSensor(0x25,0x00); //step
			CamWriteCmosSensor(0x26,0x7d);

			CamWriteCmosSensor(0x27,0x01); //30fps
			CamWriteCmosSensor(0x28,0x77);
			CamWriteCmosSensor(0x29,0x02); //20fps
			CamWriteCmosSensor(0x2a,0xee);
			CamWriteCmosSensor(0x2b,0x05); //10fps
			CamWriteCmosSensor(0x2c,0xdc);
			CamWriteCmosSensor(0x2d,0x0b); //5fps
			CamWriteCmosSensor(0x2e,0xb8);
			CamWriteCmosSensor(0x3c,0x00);
			CamWriteCmosSensor(0xfe,0x00);

		}
		else
		{
		           LOG_INF("sensor do not support the banding");
		}
        	}
	else if  ( 15 == u2FrameRate)
	{
           LOG_INF(" fix 15 fps");

		if ( GC0310_BANDING_50HZ == bBanding )
		{
			CamWriteCmosSensor(0xfe,0x00);
			CamWriteCmosSensor(0x05,0x00);
			CamWriteCmosSensor(0x06,0x6a);
			CamWriteCmosSensor(0x07,0x01);
			CamWriteCmosSensor(0x08,0xfe);

			CamWriteCmosSensor(0xfe,0x01);
			CamWriteCmosSensor(0x25,0x00); //step
			CamWriteCmosSensor(0x26,0x96);

			CamWriteCmosSensor(0x27,0x03); //15fps
			CamWriteCmosSensor(0x28,0x84);
			CamWriteCmosSensor(0x29,0x03); //14.5fps
			CamWriteCmosSensor(0x2a,0x84);
			CamWriteCmosSensor(0x2b,0x03); //14.5fps
			CamWriteCmosSensor(0x2c,0x84);
			CamWriteCmosSensor(0x2d,0x03); //14.5fps
			CamWriteCmosSensor(0x2e,0x84);
			CamWriteCmosSensor(0x3c,0x00);
			CamWriteCmosSensor(0xfe,0x00);

		}
		else if ( GC0310_BANDING_60HZ == bBanding  )
		{
			CamWriteCmosSensor(0xfe,0x00);
			CamWriteCmosSensor(0x05,0x00);
			CamWriteCmosSensor(0x06,0x6a);
			CamWriteCmosSensor(0x07,0x02);
			CamWriteCmosSensor(0x08,0x00);

			CamWriteCmosSensor(0xfe,0x01);
			CamWriteCmosSensor(0x25,0x00); //step
			CamWriteCmosSensor(0x26,0x7d);

			CamWriteCmosSensor(0x27,0x03); //15fps
			CamWriteCmosSensor(0x28,0xe8);
			CamWriteCmosSensor(0x29,0x03); //15fps
			CamWriteCmosSensor(0x2a,0xe8);
			CamWriteCmosSensor(0x2b,0x03); //15fps
			CamWriteCmosSensor(0x2c,0xe8);
			CamWriteCmosSensor(0x2d,0x03); //15fps
			CamWriteCmosSensor(0x2e,0xe8);
			CamWriteCmosSensor(0x3c,0x00);
			CamWriteCmosSensor(0xfe,0x00);


		}
		else
		{
		           LOG_INF("sensor do not support the banding");
		}
        	}
	else
	{
	            LOG_INF("sensor do not support the Frame Rate");

	};
	LOG_INF("shutter %d\n",GC0310_Read_Shutter());

}
void set_freeframerate_by_banding(UINT16 u2minFrameRate, UINT16 u2maxFrameRate,  kal_bool bBanding)
{
	if ((5 == u2minFrameRate) && (30 == u2maxFrameRate))
	{
		if ( GC0310_BANDING_50HZ == bBanding )
		{
			CamWriteCmosSensor(0xfe,0x00);
			CamWriteCmosSensor(0x05,0x00);
			CamWriteCmosSensor(0x06,0x6a);
			CamWriteCmosSensor(0x07,0x00);
			CamWriteCmosSensor(0x08,0x0a);

			CamWriteCmosSensor(0xfe,0x01);
			CamWriteCmosSensor(0x25,0x00); //step
			CamWriteCmosSensor(0x26,0x96);

			CamWriteCmosSensor(0x27,0x01); //30fps
			CamWriteCmosSensor(0x28,0xc2);
			CamWriteCmosSensor(0x29,0x02); //20fps
			CamWriteCmosSensor(0x2a,0xee);
			CamWriteCmosSensor(0x2b,0x05); //10fps
			CamWriteCmosSensor(0x2c,0xdc);
			CamWriteCmosSensor(0x2d,0x0b); //5fps
			CamWriteCmosSensor(0x2e,0xb8);
			CamWriteCmosSensor(0x3c,0x30);
			CamWriteCmosSensor(0xfe,0x00);
		}
		else if ( GC0310_BANDING_60HZ == bBanding  )
		{
			CamWriteCmosSensor(0xfe,0x00);
			CamWriteCmosSensor(0x05,0x00);
			CamWriteCmosSensor(0x06,0x6a);
			CamWriteCmosSensor(0x07,0x00);
			CamWriteCmosSensor(0x08,0x0a);

			CamWriteCmosSensor(0xfe,0x01);
			CamWriteCmosSensor(0x25,0x00); //step
			CamWriteCmosSensor(0x26,0x7d);

			CamWriteCmosSensor(0x27,0x01); //30fps
			CamWriteCmosSensor(0x28,0x77);
			CamWriteCmosSensor(0x29,0x02); //20fps
			CamWriteCmosSensor(0x2a,0xee);
			CamWriteCmosSensor(0x2b,0x05); //10fps
			CamWriteCmosSensor(0x2c,0xdc);
			CamWriteCmosSensor(0x2d,0x0b); //5fps
			CamWriteCmosSensor(0x2e,0xb8);
			CamWriteCmosSensor(0x3c,0x30);
			CamWriteCmosSensor(0xfe,0x00);
		}
		else
		{
	            LOG_INF("sensor do not support the banding");
		}
	}
	else if ((10 == u2minFrameRate) && (30 == u2maxFrameRate))
	{
		if ( GC0310_BANDING_50HZ == bBanding )
		{
			CamWriteCmosSensor(0xfe,0x00);
			CamWriteCmosSensor(0x05,0x00);
			CamWriteCmosSensor(0x06,0x6a);
			CamWriteCmosSensor(0x07,0x00);
			CamWriteCmosSensor(0x08,0x0a);

			CamWriteCmosSensor(0xfe,0x01);
			CamWriteCmosSensor(0x25,0x00); //step
			CamWriteCmosSensor(0x26,0x96);

			CamWriteCmosSensor(0x27,0x01); //30fps
			CamWriteCmosSensor(0x28,0xc2);
			CamWriteCmosSensor(0x29,0x02); //20fps
			CamWriteCmosSensor(0x2a,0xee);
			CamWriteCmosSensor(0x2b,0x05); //10fps
			CamWriteCmosSensor(0x2c,0xdc);
			CamWriteCmosSensor(0x2d,0x05); //10fps
			CamWriteCmosSensor(0x2e,0xdc);
			CamWriteCmosSensor(0x3c,0x30);
			CamWriteCmosSensor(0xfe,0x00);
		}
		else if ( GC0310_BANDING_60HZ == bBanding  )
		{
			CamWriteCmosSensor(0xfe,0x00);
			CamWriteCmosSensor(0x05,0x00);
			CamWriteCmosSensor(0x06,0x6a);
			CamWriteCmosSensor(0x07,0x00);
			CamWriteCmosSensor(0x08,0x0a);

			CamWriteCmosSensor(0xfe,0x01);
			CamWriteCmosSensor(0x25,0x00); //step
			CamWriteCmosSensor(0x26,0x7d);

			CamWriteCmosSensor(0x27,0x01); //30fps
			CamWriteCmosSensor(0x28,0x77);
			CamWriteCmosSensor(0x29,0x02); //20fps
			CamWriteCmosSensor(0x2a,0xee);
			CamWriteCmosSensor(0x2b,0x05); //10fps
			CamWriteCmosSensor(0x2c,0xdc);
			CamWriteCmosSensor(0x2d,0x05); //10fps
			CamWriteCmosSensor(0x2e,0xdc);
			CamWriteCmosSensor(0x3c,0x30);
			CamWriteCmosSensor(0xfe,0x00);
		}
		else
		{
		            LOG_INF("sensor do not support the banding");

		}
	}
	LOG_INF("shutter %d\n",GC0310_Read_Shutter());
}

UINT32 GC0310YUVSetVideoMode(UINT16 u2FrameRate)    // lanking add
{

    LOG_INF(" GC0310YUVSetVideoMode, u2FrameRate = %d\n", u2FrameRate);


     if (u2FrameRate == 30)
    {
         LOG_INF(" fix30fps,GC0310_banding_state = %d\n", GC0310_banding_state);

	set_fixframerate_by_banding(30, GC0310_banding_state);

    }
    else if (u2FrameRate == 15)
        {
           LOG_INF("fix15fps, GC0310_banding_state = %d\n", GC0310_banding_state);

       	set_fixframerate_by_banding(15, GC0310_banding_state);

	}
    else
    {

            LOG_INF("Wrong Frame Rate");

    }
    LOG_INF("GC0310YUVSetVideoMode EXIT\n");

    return TRUE;

}


/*************************************************************************
 * FUNCTION
 *  GC0310_NightMode
 *
 * DESCRIPTION
 *  This function night mode of GC0310.
 *
 * PARAMETERS
 *  bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
 *
 * RETURNS
 *  None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC0310NightMode(kal_bool bEnable)
{
	LOG_INF("GC0310Night, Modeenable(%d)\n", bEnable);
         LOG_INF("GC0310_banding_state = %d\n", GC0310_banding_state);


	if(bEnable) // night mode for preview and video
	{
        LOG_INF("set_night_mode for preview\n");
        set_freeframerate_by_banding(5, 30, GC0310_banding_state);
	}
	else  // normal mode for preview and video (not night)
	{
        LOG_INF("set_normal_mode for preview\n");
        set_freeframerate_by_banding(10, 30, GC0310_banding_state);
	}

         LOG_INF("set_night_mode end \n");
}


UINT32 GC0310YUVSensorSetting(FEATURE_ID iCmd, UINT16 iPara)
{
    switch (iCmd) {
    case FID_AWB_MODE:
        GC0310_set_param_wb(iPara);
        break;
    case FID_COLOR_EFFECT:
        GC0310_set_param_effect(iPara);
        break;
    case FID_AE_EV:
        GC0310_set_param_exposure(iPara);
        break;
    case FID_AE_FLICKER:
        GC0310_set_param_banding(iPara);
        break;
    case FID_SCENE_MODE:
        GC0310NightMode(iPara);
        break;

    case FID_ISP_CONTRAST:
        GC0310_set_contrast(iPara);
        break;
    case FID_ISP_BRIGHT:
        GC0310_set_brightness(iPara);
        break;
    case FID_ISP_SAT:
        GC0310_set_saturation(iPara);
        break;
    case FID_AE_ISO:
        GC0310_set_iso(iPara);
        break;
    case FID_AE_SCENE_MODE:
        GC0310_set_AE_mode(iPara);
        break;

    default:
        break;
    }
    return TRUE;
} /* GC0310YUVSensorSetting */


UINT32 GC0310FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
        UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    UINT32 **ppFeatureData=(UINT32 **) pFeaturePara;
    unsigned long long *feature_data=(unsigned long long *) pFeaturePara;
    unsigned long long *feature_return_para=(unsigned long long *) pFeaturePara;

    UINT32 GC0310SensorRegNumber;
    UINT32 i;
    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
    LOG_INF("FeatureId = %d", FeatureId);

    switch (FeatureId)
    {
    case SENSOR_FEATURE_GET_RESOLUTION:
        *pFeatureReturnPara16++=IMAGE_SENSOR_FULL_WIDTH;
        *pFeatureReturnPara16=IMAGE_SENSOR_FULL_HEIGHT;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PERIOD:
        *pFeatureReturnPara16++=(VGA_PERIOD_PIXEL_NUMS)+GC0310_dummy_pixels;
        *pFeatureReturnPara16=(VGA_PERIOD_LINE_NUMS)+GC0310_dummy_lines;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        *pFeatureReturnPara32 = GC0310_g_fPV_PCLK;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_ESHUTTER:
        break;
    case SENSOR_FEATURE_SET_NIGHTMODE:
 //       GC0310NightMode((BOOL) *feature_data);
        break;
    case SENSOR_FEATURE_SET_GAIN:
    case SENSOR_FEATURE_SET_FLASHLIGHT:
        break;
    case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
        GC0310_isp_master_clock=*pFeatureData32;
        break;
    case SENSOR_FEATURE_SET_REGISTER:
        GC0310_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
        break;
    case SENSOR_FEATURE_GET_REGISTER:
        pSensorRegData->RegData = GC0310_read_cmos_sensor(pSensorRegData->RegAddr);
        break;
    case SENSOR_FEATURE_GET_CONFIG_PARA:
        memcpy(pSensorConfigData, &GC0310SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
        *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
        break;
    case SENSOR_FEATURE_SET_CCT_REGISTER:
    case SENSOR_FEATURE_GET_CCT_REGISTER:
    case SENSOR_FEATURE_SET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
    case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
    case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
    case SENSOR_FEATURE_GET_GROUP_COUNT:
    case SENSOR_FEATURE_GET_GROUP_INFO:
    case SENSOR_FEATURE_GET_ITEM_INFO:
    case SENSOR_FEATURE_SET_ITEM_INFO:
    case SENSOR_FEATURE_GET_ENG_INFO:
        break;
    case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
        // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
        // if EEPROM does not exist in camera module.
        *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_YUV_CMD:
        GC0310YUVSensorSetting((FEATURE_ID)*feature_data, *(feature_data+1));

        break;
    case SENSOR_FEATURE_SET_VIDEO_MODE:    //  lanking
	 GC0310YUVSetVideoMode(*feature_data);
	 break;
    case SENSOR_FEATURE_CHECK_SENSOR_ID:
	GC0310GetSensorID(pFeatureData32);
	break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		*pFeatureReturnPara32=GC0310_TEST_PATTERN_CHECKSUM;
		*pFeatureParaLen=4;
		break;

	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		 LOG_INF("[GC0310] F_SET_MAX_FRAME_RATE_BY_SCENARIO.\n");
		 GC0310_MIPI_SetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*feature_data, *(feature_data+1));
		 break;

//	case SENSOR_CMD_SET_VIDEO_FRAME_RATE:
//		LOG_INF("[GC0310] Enter SENSOR_CMD_SET_VIDEO_FRAME_RATE\n");
//		//GC0310_MIPI_SetVideoFrameRate(*pFeatureData32);
//		break;

	case SENSOR_FEATURE_SET_TEST_PATTERN:
		GC0310SetTestPatternMode((BOOL)*feature_data);
		break;

    case SENSOR_FEATURE_GET_DELAY_INFO:
        LOG_INF("[GC0310] F_GET_DELAY_INFO\n");
        GC0310_MIPI_GetDelayInfo((uintptr_t)*feature_data);
    break;

    case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
         LOG_INF("[GC0310] F_GET_DEFAULT_FRAME_RATE_BY_SCENARIO\n");
         GC0310_MIPI_GetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*feature_data, (MUINT32 *)(uintptr_t)(*(feature_data+1)));
    break;

	case SENSOR_FEATURE_SET_YUV_3A_CMD:
		 LOG_INF("[GC0310] SENSOR_FEATURE_SET_YUV_3A_CMD ID:%d\n", *pFeatureData32);
		 GC0310_3ACtrl((ACDK_SENSOR_3A_LOCK_ENUM)*feature_data);
		 break;


	case SENSOR_FEATURE_GET_EXIF_INFO:
		 //LOG_INF("[4EC] F_GET_EXIF_INFO\n");
		 GC0310_MIPI_GetExifInfo((uintptr_t)*feature_data);
		 break;


	case SENSOR_FEATURE_GET_AE_AWB_LOCK_INFO:
		 LOG_INF("[GC0310] F_GET_AE_AWB_LOCK_INFO\n");
		 //GC0310_MIPI_get_AEAWB_lock((uintptr_t)(*feature_data), (uintptr_t)*(feature_data+1));
	break;

	case SENSOR_FEATURE_SET_MIN_MAX_FPS:
		LOG_INF("SENSOR_FEATURE_SET_MIN_MAX_FPS:[%d,%d]\n",*pFeatureData32,*(pFeatureData32+1));
		GC0310_MIPI_SetMaxMinFps((UINT32)*feature_data, (UINT32)*(feature_data+1));
	break;

    default:
        break;
	}
return ERROR_NONE;
}   /* GC0310FeatureControl() */


SENSOR_FUNCTION_STRUCT  SensorFuncGC0310YUV=
{
    GC0310Open,
    GC0310GetInfo,
    GC0310GetResolution,
    GC0310FeatureControl,
    GC0310Control,
    GC0310Close
};


UINT32 GC0310_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncGC0310YUV;
    return ERROR_NONE;
} /* SensorInit() */
