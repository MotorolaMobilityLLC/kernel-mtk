/*
 * Copyright (C) 2008 MediaTek Inc.
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

 ****************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <asm/system.h>


#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "imx178mipiraw_Sensor.h"
#include "imx178mipiraw_Camera_Sensor_para.h"
#include "imx178mipiraw_CameraCustomized.h"

#define IMX178_USE_OTP

#ifdef IMX178_USE_OTP
#define IMX178_USE_AWB_OTP
static uint16_t used_otp = 0;
extern imx178_update_awb_gain();
extern imx178_update_otp_wb();
extern get_sensor_imx178_customer_id();
#endif


kal_bool  IMX178MIPI_MPEG4_encode_mode = KAL_FALSE;
kal_bool IMX178MIPI_Auto_Flicker_mode = KAL_FALSE;


kal_uint8 IMX178MIPI_sensor_write_I2C_address = IMX178MIPI_WRITE_ID;
kal_uint8 IMX178MIPI_sensor_read_I2C_address = IMX178MIPI_READ_ID;


	
static struct IMX178MIPI_sensor_STRUCT IMX178MIPI_sensor={IMX178MIPI_WRITE_ID,IMX178MIPI_READ_ID,KAL_TRUE,KAL_FALSE,KAL_TRUE,KAL_FALSE,
KAL_FALSE,KAL_FALSE,KAL_FALSE,176800000,176800000,176800000,0,0,0,64,64,64,IMX178MIPI_PV_LINE_LENGTH_PIXELS,IMX178MIPI_PV_FRAME_LENGTH_LINES,
IMX178MIPI_VIDEO_LINE_LENGTH_PIXELS,IMX178MIPI_VIDEO_FRAME_LENGTH_LINES,IMX178MIPI_FULL_LINE_LENGTH_PIXELS,IMX178MIPI_FULL_FRAME_LENGTH_LINES,0,0,0,0,0,0,30};
static MSDK_SCENARIO_ID_ENUM CurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;		
kal_uint16	IMX178MIPI_sensor_gain_base=0x0;

/* MAX/MIN Explosure Lines Used By AE Algorithm */
kal_uint16 IMX178MIPI_MAX_EXPOSURE_LINES = IMX178MIPI_PV_FRAME_LENGTH_LINES-5;//650;
kal_uint8  IMX178MIPI_MIN_EXPOSURE_LINES = 2;
kal_uint32 IMX178MIPI_isp_master_clock;
static DEFINE_SPINLOCK(imx178_drv_lock);

#define SENSORDB(fmt, arg...) printk( "[IMX178MIPIRaw] "  fmt, ##arg)
#define RETAILMSG(x,...)
#define TEXT
UINT8 IMX178MIPIPixelClockDivider=0;
kal_uint16 IMX178MIPI_sensor_id=0;
MSDK_SENSOR_CONFIG_STRUCT IMX178MIPISensorConfigData;
kal_uint32 IMX178MIPI_FAC_SENSOR_REG;
kal_uint16 IMX178MIPI_sensor_flip_value; 
#define IMX178MIPI_MaxGainIndex (97)
kal_uint16 IMX178MIPI_sensorGainMapping[IMX178MIPI_MaxGainIndex][2] ={
{ 64 ,0  },   
{ 68 ,12 },   
{ 71 ,23 },   
{ 74 ,33 },   
{ 77 ,42 },   
{ 81 ,52 },   
{ 84 ,59 },   
{ 87 ,66 },   
{ 90 ,73 },   
{ 93 ,79 },   
{ 96 ,85 },   
{ 100,91 },   
{ 103,96 },   
{ 106,101},   
{ 109,105},   
{ 113,110},   
{ 116,114},   
{ 120,118},   
{ 122,121},   
{ 125,125},   
{ 128,128},   
{ 132,131},   
{ 135,134},   
{ 138,137},
{ 141,139},
{ 144,142},   
{ 148,145},   
{ 151,147},   
{ 153,149}, 
{ 157,151},
{ 160,153},      
{ 164,156},   
{ 168,158},   
{ 169,159},   
{ 173,161},   
{ 176,163},   
{ 180,165}, 
{ 182,166},   
{ 187,168},
{ 189,169},
{ 193,171},
{ 196,172},
{ 200,174},
{ 203,175}, 
{ 205,176},
{ 208,177}, 
{ 213,179}, 
{ 216,180},  
{ 219,181},   
{ 222,182},
{ 225,183},  
{ 228,184},   
{ 232,185},
{ 235,186},
{ 238,187},
{ 241,188},
{ 245,189},
{ 249,190},
{ 253,191},
{ 256,192}, 
{ 260,193},
{ 265,194},
{ 269,195},
{ 274,196},   
{ 278,197},
{ 283,198},
{ 288,199},
{ 293,200},
{ 298,201},   
{ 304,202},   
{ 310,203},
{ 315,204},
{ 322,205},   
{ 328,206},   
{ 335,207},   
{ 342,208},   
{ 349,209},   
{ 357,210},   
{ 365,211},   
{ 373,212}, 
{ 381,213},
{ 400,215},      
{ 420,217},   
{ 432,218},   
{ 443,219},      
{ 468,221},   
{ 482,222},   
{ 497,223},   
{ 512,224},
{ 529,225}, 	 
{ 546,226},   
{ 566,227},   
{ 585,228}, 	 
{ 607,229},   
{ 631,230},   
{ 656,231},   
{ 683,232}
};
/* FIXME: old factors and DIDNOT use now. s*/
SENSOR_REG_STRUCT IMX178MIPISensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT IMX178MIPISensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
/* FIXME: old factors and DIDNOT use now. e*/
extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
#define IMX178MIPI_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1, IMX178MIPI_WRITE_ID)

kal_uint16 IMX178MIPI_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,IMX178MIPI_WRITE_ID);
    return get_byte;
}

void IMX178MIPI_write_shutter(kal_uint16 shutter)
{
	kal_uint32 frame_length = 0,line_length=0,shutter1=0;
    kal_uint32 extra_lines = 0;
	kal_uint32 max_exp_shutter = 0;
	unsigned long flags;
	
	SENSORDB("[IMX178MIPI]enter IMX178MIPI_write_shutter function\n"); 
    if (IMX178MIPI_sensor.pv_mode == KAL_TRUE) 
	 {
	   max_exp_shutter = IMX178MIPI_PV_FRAME_LENGTH_LINES + IMX178MIPI_sensor.pv_dummy_lines-5;
     }
     else if (IMX178MIPI_sensor.video_mode== KAL_TRUE) 
     {
       max_exp_shutter = IMX178MIPI_VIDEO_FRAME_LENGTH_LINES + IMX178MIPI_sensor.video_dummy_lines-5;
	 }	 
     else if (IMX178MIPI_sensor.capture_mode== KAL_TRUE) 
     {
       max_exp_shutter = IMX178MIPI_FULL_FRAME_LENGTH_LINES + IMX178MIPI_sensor.cp_dummy_lines-5;
	 }	 
	 else
	 	{
	 	
		SENSORDB("sensor mode error\n");
	 	}
	 
	 if(shutter > max_exp_shutter)
	   extra_lines = shutter - max_exp_shutter;
	 else 
	   extra_lines = 0;
	 if (IMX178MIPI_sensor.pv_mode == KAL_TRUE) 
	 {
       frame_length =IMX178MIPI_PV_FRAME_LENGTH_LINES+ IMX178MIPI_sensor.pv_dummy_lines + extra_lines;
	   line_length = IMX178MIPI_PV_LINE_LENGTH_PIXELS+ IMX178MIPI_sensor.pv_dummy_pixels;
	   spin_lock_irqsave(&imx178_drv_lock,flags);
	   IMX178MIPI_sensor.pv_line_length = line_length;
	   IMX178MIPI_sensor.pv_frame_length = frame_length;
	   spin_unlock_irqrestore(&imx178_drv_lock,flags);
	 }
	 else if (IMX178MIPI_sensor.video_mode== KAL_TRUE) 
     {
	    frame_length = IMX178MIPI_VIDEO_FRAME_LENGTH_LINES+ IMX178MIPI_sensor.video_dummy_lines + extra_lines;
		line_length =IMX178MIPI_VIDEO_LINE_LENGTH_PIXELS + IMX178MIPI_sensor.video_dummy_pixels;
		spin_lock_irqsave(&imx178_drv_lock,flags);
		IMX178MIPI_sensor.video_line_length = line_length;
	    IMX178MIPI_sensor.video_frame_length = frame_length;
		spin_unlock_irqrestore(&imx178_drv_lock,flags);
	 } 
	 else if(IMX178MIPI_sensor.capture_mode== KAL_TRUE)
	 	{
	    frame_length = IMX178MIPI_FULL_FRAME_LENGTH_LINES+ IMX178MIPI_sensor.cp_dummy_lines + extra_lines;
		line_length =IMX178MIPI_FULL_LINE_LENGTH_PIXELS + IMX178MIPI_sensor.cp_dummy_pixels;
		spin_lock_irqsave(&imx178_drv_lock,flags);
		IMX178MIPI_sensor.cp_line_length = line_length;
	    IMX178MIPI_sensor.cp_frame_length = frame_length;
		spin_unlock_irqrestore(&imx178_drv_lock,flags);
	 }
	 else
	 	{
	 	
		SENSORDB("sensor mode error\n");
	 	}
	//IMX178MIPI_write_cmos_sensor(0x0100,0x00);// STREAM STop
	IMX178MIPI_write_cmos_sensor(0x0104, 1);        
	IMX178MIPI_write_cmos_sensor(0x0340, (frame_length >>8) & 0xFF);
    IMX178MIPI_write_cmos_sensor(0x0341, frame_length & 0xFF);	
    
    IMX178MIPI_write_cmos_sensor(0x0202, (shutter >> 8) & 0xFF);
    IMX178MIPI_write_cmos_sensor(0x0203, shutter  & 0xFF);
    IMX178MIPI_write_cmos_sensor(0x0104, 0);    
    SENSORDB("[IMX178MIPI]exit IMX178MIPI_write_shutter function\n");

}   /* write_IMX178MIPI_shutter */

static kal_uint16 IMX178MIPIReg2Gain(const kal_uint8 iReg)
{
	SENSORDB("[IMX178MIPI]enter IMX178MIPIReg2Gain function\n");
    kal_uint8 iI;
    // Range: 1x to 8x
    for (iI = 0; iI < IMX178MIPI_MaxGainIndex; iI++) 
	{
        if(iReg < IMX178MIPI_sensorGainMapping[iI][1])
		{
            break;
        }
		if(iReg == IMX178MIPI_sensorGainMapping[iI][1])			
		{			
			return IMX178MIPI_sensorGainMapping[iI][0];
		}    
    }
	SENSORDB("[IMX178MIPI]exit IMX178MIPIReg2Gain function\n");
    return IMX178MIPI_sensorGainMapping[iI-1][0];
}
static kal_uint8 IMX178MIPIGain2Reg(const kal_uint16 iGain)
{
	kal_uint8 iI;
    SENSORDB("[IMX178MIPI]enter IMX178MIPIGain2Reg function\n");
    for (iI = 0; iI < (IMX178MIPI_MaxGainIndex-1); iI++) 
	{
        if(iGain <IMX178MIPI_sensorGainMapping[iI][0])
		{    
            break;
        }
		if(iGain < IMX178MIPI_sensorGainMapping[iI][0])
		{                
			return IMX178MIPI_sensorGainMapping[iI][1];       
		}
			
    }
    if(iGain != IMX178MIPI_sensorGainMapping[iI][0])
    {
         printk("[IMX178MIPIGain2Reg] Gain mapping don't correctly:%d %d \n", iGain, IMX178MIPI_sensorGainMapping[iI][0]);
    }
	SENSORDB("[IMX178MIPI]exit IMX178MIPIGain2Reg function\n");
    return IMX178MIPI_sensorGainMapping[iI-1][1];
	//return NONE;
}

/*************************************************************************
* FUNCTION
*    IMX178MIPI_SetGain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    gain : sensor global gain(base: 0x40)
*
* RETURNS
*    the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
void IMX178MIPI_SetGain(UINT16 iGain)
{   
    kal_uint8 iReg;
	SENSORDB("[IMX178MIPI]enter IMX178MIPI_SetGain function\n");
    iReg = IMX178MIPIGain2Reg(iGain);
	IMX178MIPI_write_cmos_sensor(0x0104, 1);
    IMX178MIPI_write_cmos_sensor(0x0205, (kal_uint8)iReg);
    IMX178MIPI_write_cmos_sensor(0x0104, 0);
    SENSORDB("[IMX178MIPI]exit IMX178MIPI_SetGain function\n");
}   /*  IMX178MIPI_SetGain_SetGain  */


/*************************************************************************
* FUNCTION
*    read_IMX178MIPI_gain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain(base: 0x40)
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_uint16 read_IMX178MIPI_gain(void)
{  
	SENSORDB("[IMX178MIPI]enter read_IMX178MIPI_gain function\n");
    return (kal_uint16)((IMX178MIPI_read_cmos_sensor(0x0204)<<8) | IMX178MIPI_read_cmos_sensor(0x0205)) ;
}  /* read_IMX178MIPI_gain */

void write_IMX178MIPI_gain(kal_uint16 gain)
{
    IMX178MIPI_SetGain(gain);
}
void IMX178MIPI_camera_para_to_sensor(void)
{

	kal_uint32    i;
	SENSORDB("[IMX178MIPI]enter IMX178MIPI_camera_para_to_sensor function\n");
    for(i=0; 0xFFFFFFFF!=IMX178MIPISensorReg[i].Addr; i++)
    {
        IMX178MIPI_write_cmos_sensor(IMX178MIPISensorReg[i].Addr, IMX178MIPISensorReg[i].Para);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=IMX178MIPISensorReg[i].Addr; i++)
    {
        IMX178MIPI_write_cmos_sensor(IMX178MIPISensorReg[i].Addr, IMX178MIPISensorReg[i].Para);
    }
    for(i=FACTORY_START_ADDR; i<FACTORY_END_ADDR; i++)
    {
        IMX178MIPI_write_cmos_sensor(IMX178MIPISensorCCT[i].Addr, IMX178MIPISensorCCT[i].Para);
    }
	SENSORDB("[IMX178MIPI]exit IMX178MIPI_camera_para_to_sensor function\n");

}


/*************************************************************************
* FUNCTION
*    IMX178MIPI_sensor_to_camera_para
*
* DESCRIPTION
*    // update camera_para from sensor register
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain(base: 0x40)
*
* GLOBALS AFFECTED
*
*************************************************************************/
void IMX178MIPI_sensor_to_camera_para(void)
{

	kal_uint32    i,temp_data;
	SENSORDB("[IMX178MIPI]enter IMX178MIPI_sensor_to_camera_para function\n");
    for(i=0; 0xFFFFFFFF!=IMX178MIPISensorReg[i].Addr; i++)
    {
		temp_data=IMX178MIPI_read_cmos_sensor(IMX178MIPISensorReg[i].Addr);
		spin_lock(&imx178_drv_lock);
		IMX178MIPISensorReg[i].Para = temp_data;
		spin_unlock(&imx178_drv_lock);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=IMX178MIPISensorReg[i].Addr; i++)
    {
    	temp_data=IMX178MIPI_read_cmos_sensor(IMX178MIPISensorReg[i].Addr);
         spin_lock(&imx178_drv_lock);
        IMX178MIPISensorReg[i].Para = temp_data;
		spin_unlock(&imx178_drv_lock);
    }
	SENSORDB("[IMX178MIPI]exit IMX178MIPI_sensor_to_camera_para function\n");

}

/*************************************************************************
* FUNCTION
*    IMX178MIPI_get_sensor_group_count
*
* DESCRIPTION
*    //
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain(base: 0x40)
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_int32  IMX178MIPI_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void IMX178MIPI_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
{
   switch (group_idx)
   {
        case PRE_GAIN:
            sprintf((char *)group_name_ptr, "CCT");
            *item_count_ptr = 2;
            break;
        case CMMCLK_CURRENT:
            sprintf((char *)group_name_ptr, "CMMCLK Current");
            *item_count_ptr = 1;
            break;
        case FRAME_RATE_LIMITATION:
            sprintf((char *)group_name_ptr, "Frame Rate Limitation");
            *item_count_ptr = 2;
            break;
        case REGISTER_EDITOR:
            sprintf((char *)group_name_ptr, "Register Editor");
            *item_count_ptr = 2;
            break;
        default:
            ASSERT(0);
}
}

void IMX178MIPI_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
{
    kal_int16 temp_reg=0;
    kal_uint16 temp_gain=0, temp_addr=0, temp_para=0;
    
    switch (group_idx)
    {
        case PRE_GAIN:
            switch (item_idx)
            {
              case 0:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-R");
                  temp_addr = PRE_GAIN_R_INDEX;
              break;
              case 1:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-Gr");
                  temp_addr = PRE_GAIN_Gr_INDEX;
              break;
              case 2:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-Gb");
                  temp_addr = PRE_GAIN_Gb_INDEX;
              break;
              case 3:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-B");
                  temp_addr = PRE_GAIN_B_INDEX;
              break;
              case 4:
                 sprintf((char *)info_ptr->ItemNamePtr,"SENSOR_BASEGAIN");
                 temp_addr = SENSOR_BASEGAIN;
              break;
              default:
                 SENSORDB("[IMX105MIPI][Error]get_sensor_item_info error!!!\n");
          }
           	spin_lock(&imx178_drv_lock);    
            temp_para=IMX178MIPISensorCCT[temp_addr].Para;	
			spin_unlock(&imx178_drv_lock);
            temp_gain = IMX178MIPIReg2Gain(temp_para);
            temp_gain=(temp_gain*1000)/BASEGAIN;
            info_ptr->ItemValue=temp_gain;
            info_ptr->IsTrueFalse=KAL_FALSE;
            info_ptr->IsReadOnly=KAL_FALSE;
            info_ptr->IsNeedRestart=KAL_FALSE;
            info_ptr->Min=1000;
            info_ptr->Max=15875;
            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
                    sprintf((char *)info_ptr->ItemNamePtr,"Drv Cur[2,4,6,8]mA");
                
                    //temp_reg=IMX178MIPISensorReg[CMMCLK_CURRENT_INDEX].Para;
                    temp_reg = ISP_DRIVING_2MA;
                    if(temp_reg==ISP_DRIVING_2MA)
                    {
                        info_ptr->ItemValue=2;
                    }
                    else if(temp_reg==ISP_DRIVING_4MA)
                    {
                        info_ptr->ItemValue=4;
                    }
                    else if(temp_reg==ISP_DRIVING_6MA)
                    {
                        info_ptr->ItemValue=6;
                    }
                    else if(temp_reg==ISP_DRIVING_8MA)
                    {
                        info_ptr->ItemValue=8;
                    }
                
                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_FALSE;
                    info_ptr->IsNeedRestart=KAL_TRUE;
                    info_ptr->Min=2;
                    info_ptr->Max=8;
                    break;
                default:
                    ASSERT(0);
            }
            break;
        case FRAME_RATE_LIMITATION:
            switch (item_idx)
            {
                case 0:
                    sprintf((char *)info_ptr->ItemNamePtr,"Max Exposure Lines");
                    info_ptr->ItemValue=IMX178MIPI_MAX_EXPOSURE_LINES;
                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_TRUE;
                    info_ptr->IsNeedRestart=KAL_FALSE;
                    info_ptr->Min=0;
                    info_ptr->Max=0;
                    break;
                case 1:
                    sprintf((char *)info_ptr->ItemNamePtr,"Min Frame Rate");
                    info_ptr->ItemValue=12;
                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_TRUE;
                    info_ptr->IsNeedRestart=KAL_FALSE;
                    info_ptr->Min=0;
                    info_ptr->Max=0;
                    break;
                default:
                    ASSERT(0);
            }
            break;
        case REGISTER_EDITOR:
            switch (item_idx)
            {
                case 0:
                    sprintf((char *)info_ptr->ItemNamePtr,"REG Addr.");
                    info_ptr->ItemValue=0;
                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_FALSE;
                    info_ptr->IsNeedRestart=KAL_FALSE;
                    info_ptr->Min=0;
                    info_ptr->Max=0xFFFF;
                    break;
                case 1:
                    sprintf((char *)info_ptr->ItemNamePtr,"REG Value");
                    info_ptr->ItemValue=0;
                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_FALSE;
                    info_ptr->IsNeedRestart=KAL_FALSE;
                    info_ptr->Min=0;
                    info_ptr->Max=0xFFFF;
                    break;
                default:
                ASSERT(0);
            }
            break;
        default:
            ASSERT(0);
    }
}
kal_bool IMX178MIPI_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
{
   kal_uint16 temp_addr=0, temp_para=0;

   switch (group_idx)
    {
        case PRE_GAIN:
            switch (item_idx)
            {
              case 0:
                temp_addr = PRE_GAIN_R_INDEX;
              break;
              case 1:
                temp_addr = PRE_GAIN_Gr_INDEX;
              break;
              case 2:
                temp_addr = PRE_GAIN_Gb_INDEX;
              break;
              case 3:
                temp_addr = PRE_GAIN_B_INDEX;
              break;
              case 4:
                temp_addr = SENSOR_BASEGAIN;
              break;
              default:
                 SENSORDB("[IMX105MIPI][Error]set_sensor_item_info error!!!\n");
          }
            temp_para = IMX178MIPIGain2Reg(ItemValue);
            spin_lock(&imx178_drv_lock);    
            IMX178MIPISensorCCT[temp_addr].Para = temp_para;
			spin_unlock(&imx178_drv_lock);
            IMX178MIPI_write_cmos_sensor(IMX178MIPISensorCCT[temp_addr].Addr,temp_para);
			temp_para=read_IMX178MIPI_gain();	
            spin_lock(&imx178_drv_lock);    
            IMX178MIPI_sensor_gain_base=temp_para;
			spin_unlock(&imx178_drv_lock);

            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
                    if(ItemValue==2)
                    {			
                    spin_lock(&imx178_drv_lock);    
                        IMX178MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_2MA;
					spin_unlock(&imx178_drv_lock);
                        //IMX178MIPI_set_isp_driving_current(ISP_DRIVING_2MA);
                    }
                    else if(ItemValue==3 || ItemValue==4)
                    {
                    	spin_lock(&imx178_drv_lock);    
                        IMX178MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_4MA;
						spin_unlock(&imx178_drv_lock);
                        //IMX178MIPI_set_isp_driving_current(ISP_DRIVING_4MA);
                    }
                    else if(ItemValue==5 || ItemValue==6)
                    {
                    	spin_lock(&imx178_drv_lock);    
                        IMX178MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_6MA;
						spin_unlock(&imx178_drv_lock);
                        //IMX178MIPI_set_isp_driving_current(ISP_DRIVING_6MA);
                    }
                    else
                    {
                    	spin_lock(&imx178_drv_lock);    
                        IMX178MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_8MA;
						spin_unlock(&imx178_drv_lock);
                        //IMX178MIPI_set_isp_driving_current(ISP_DRIVING_8MA);
                    }
                    break;
                default:
                    ASSERT(0);
            }
            break;
        case FRAME_RATE_LIMITATION:
            ASSERT(0);
            break;
        case REGISTER_EDITOR:
            switch (item_idx)
            {
                case 0:
					spin_lock(&imx178_drv_lock);    
                    IMX178MIPI_FAC_SENSOR_REG=ItemValue;
					spin_unlock(&imx178_drv_lock);
                    break;
                case 1:
                    IMX178MIPI_write_cmos_sensor(IMX178MIPI_FAC_SENSOR_REG,ItemValue);
                    break;
                default:
                    ASSERT(0);
            }
            break;
        default:
            ASSERT(0);
    }
    return KAL_TRUE;
}
static void IMX178MIPI_SetDummy(const kal_uint16 iPixels, const kal_uint16 iLines)
{
	kal_uint32 frame_length = 0, line_length = 0;
    if(IMX178MIPI_sensor.pv_mode == KAL_TRUE)
   	{
   	 spin_lock(&imx178_drv_lock);    
   	 IMX178MIPI_sensor.pv_dummy_pixels = iPixels;
	 IMX178MIPI_sensor.pv_dummy_lines = iLines;
   	 IMX178MIPI_sensor.pv_line_length = IMX178MIPI_PV_LINE_LENGTH_PIXELS + iPixels;
	 IMX178MIPI_sensor.pv_frame_length = IMX178MIPI_PV_FRAME_LENGTH_LINES + iLines;
	 spin_unlock(&imx178_drv_lock);
	 line_length = IMX178MIPI_sensor.pv_line_length;
	 frame_length = IMX178MIPI_sensor.pv_frame_length;	 	
   	}
   else if(IMX178MIPI_sensor.video_mode== KAL_TRUE)
   	{
   	 spin_lock(&imx178_drv_lock);    
   	 IMX178MIPI_sensor.video_dummy_pixels = iPixels;
	 IMX178MIPI_sensor.video_dummy_lines = iLines;
   	 IMX178MIPI_sensor.video_line_length = IMX178MIPI_VIDEO_LINE_LENGTH_PIXELS + iPixels;
	 IMX178MIPI_sensor.video_frame_length = IMX178MIPI_VIDEO_FRAME_LENGTH_LINES + iLines;
	 spin_unlock(&imx178_drv_lock);
	 line_length = IMX178MIPI_sensor.video_line_length;
	 frame_length = IMX178MIPI_sensor.video_frame_length;
   	}

	else if(IMX178MIPI_sensor.capture_mode== KAL_TRUE) 
		{
	  spin_lock(&imx178_drv_lock);	
   	  IMX178MIPI_sensor.cp_dummy_pixels = iPixels;
	  IMX178MIPI_sensor.cp_dummy_lines = iLines;
	  IMX178MIPI_sensor.cp_line_length = IMX178MIPI_FULL_LINE_LENGTH_PIXELS + iPixels;
	  IMX178MIPI_sensor.cp_frame_length = IMX178MIPI_FULL_FRAME_LENGTH_LINES + iLines;
	   spin_unlock(&imx178_drv_lock);
	  line_length = IMX178MIPI_sensor.cp_line_length;
	  frame_length = IMX178MIPI_sensor.cp_frame_length;
    }
	else
	{
	 SENSORDB("[IMX178MIPI]%s(),sensor mode error",__FUNCTION__);
	}
      IMX178MIPI_write_cmos_sensor(0x0104, 1);        	  
      IMX178MIPI_write_cmos_sensor(0x0340, (frame_length >>8) & 0xFF);
      IMX178MIPI_write_cmos_sensor(0x0341, frame_length & 0xFF);	
      IMX178MIPI_write_cmos_sensor(0x0342, (line_length >>8) & 0xFF);
      IMX178MIPI_write_cmos_sensor(0x0343, line_length & 0xFF);
      IMX178MIPI_write_cmos_sensor(0x0104, 0);
}   /*  IMX178MIPI_SetDummy */

static void IMX178MIPI_Sensor_Init(void)
{
	SENSORDB("[IMX178MIPI]enter IMX178MIPI_Sensor_Init function\n");
	IMX178MIPI_write_cmos_sensor(0x41C0, 0x01);
	IMX178MIPI_write_cmos_sensor(0x0104, 0x01);//group
	IMX178MIPI_write_cmos_sensor(0x0100, 0x00);//STREAM OFF 	
	IMX178MIPI_write_cmos_sensor(0x0103, 0x01);//SW reset
//	IMX178MIPI_write_cmos_sensor(0x0101, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x030E, 0x01); 	   
	IMX178MIPI_write_cmos_sensor(0x0202, 0x06); 		 
	IMX178MIPI_write_cmos_sensor(0x0203, 0xAD);
	//PLL setting
	IMX178MIPI_write_cmos_sensor(0x0301, 0x0A); 		 
	IMX178MIPI_write_cmos_sensor(0x0303, 0x01); 		 
	IMX178MIPI_write_cmos_sensor(0x0305, 0x06); 		 
	IMX178MIPI_write_cmos_sensor(0x0309, 0x0A); 		 
	IMX178MIPI_write_cmos_sensor(0x030B, 0x01); 		 
	IMX178MIPI_write_cmos_sensor(0x030C, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x030D, 0xDD); 
	IMX178MIPI_write_cmos_sensor(0x030E, 0x01);

	IMX178MIPI_write_cmos_sensor(0x0340, 0x06);
	IMX178MIPI_write_cmos_sensor(0x0341, 0xB1); 		 
	IMX178MIPI_write_cmos_sensor(0x0342, 0x0D); 		
	IMX178MIPI_write_cmos_sensor(0x0343, 0x70); 		 
	IMX178MIPI_write_cmos_sensor(0x0344, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0345, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0346, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0347, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0348, 0x0C); 		
	IMX178MIPI_write_cmos_sensor(0x0349, 0xCF); 		 
	IMX178MIPI_write_cmos_sensor(0x034A, 0x09); 		 
	IMX178MIPI_write_cmos_sensor(0x034B, 0x9F); 		 
	IMX178MIPI_write_cmos_sensor(0x034C, 0x06); 		 
	IMX178MIPI_write_cmos_sensor(0x034D, 0x68); 		 
	IMX178MIPI_write_cmos_sensor(0x034E, 0x04); 		 
	IMX178MIPI_write_cmos_sensor(0x034F, 0xD0); 		 
	IMX178MIPI_write_cmos_sensor(0x0383, 0x01); 		 
	IMX178MIPI_write_cmos_sensor(0x0387, 0x01); 		
	IMX178MIPI_write_cmos_sensor(0x0390, 0x01); 		 
	IMX178MIPI_write_cmos_sensor(0x0401, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0405, 0x10); 		 
	IMX178MIPI_write_cmos_sensor(0x3020, 0x10); 		 
	IMX178MIPI_write_cmos_sensor(0x3041, 0x15); 		 
	IMX178MIPI_write_cmos_sensor(0x3042, 0x87); 		 
	IMX178MIPI_write_cmos_sensor(0x3089, 0x4F); 		 
	IMX178MIPI_write_cmos_sensor(0x3309, 0x9A); 		 
	IMX178MIPI_write_cmos_sensor(0x3344, 0x6F); 		 
	IMX178MIPI_write_cmos_sensor(0x3345, 0x1F); 		 
	IMX178MIPI_write_cmos_sensor(0x3362, 0x0A); 		 
	IMX178MIPI_write_cmos_sensor(0x3363, 0x0A); 		 
	IMX178MIPI_write_cmos_sensor(0x3364, 0x02); 		
	IMX178MIPI_write_cmos_sensor(0x3368, 0x18); 		 
	IMX178MIPI_write_cmos_sensor(0x3369, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x3370, 0x7F); 		 
	IMX178MIPI_write_cmos_sensor(0x3371, 0x37); 		 
	IMX178MIPI_write_cmos_sensor(0x3372, 0x67); 		 
	IMX178MIPI_write_cmos_sensor(0x3373, 0x3F); 		 
	IMX178MIPI_write_cmos_sensor(0x3374, 0x3F); 		 
	IMX178MIPI_write_cmos_sensor(0x3375, 0x47); 		 
	IMX178MIPI_write_cmos_sensor(0x3376, 0xCF); 		 
	IMX178MIPI_write_cmos_sensor(0x3377, 0x47); 		
	IMX178MIPI_write_cmos_sensor(0x33C8, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x33D4, 0x06); 		 
	IMX178MIPI_write_cmos_sensor(0x33D5, 0x68); 		 
	IMX178MIPI_write_cmos_sensor(0x33D6, 0x04); 	   
	IMX178MIPI_write_cmos_sensor(0x33D7, 0xD0); 	  
	IMX178MIPI_write_cmos_sensor(0x4100, 0x0E); 	   
	IMX178MIPI_write_cmos_sensor(0x4108, 0x01);
	IMX178MIPI_write_cmos_sensor(0x4109, 0x7C);
	IMX178MIPI_write_cmos_sensor(0x0104, 0x00);//group
	IMX178MIPI_write_cmos_sensor(0x0100, 0x01);//STREAM ON
	// The register only need to enable 1 time.    
	spin_lock(&imx178_drv_lock);  
	IMX178MIPI_Auto_Flicker_mode = KAL_FALSE;	  // reset the flicker status	 
	spin_unlock(&imx178_drv_lock);
	SENSORDB("[IMX178MIPI]exit PreviewSetting function\n");
}   /*  IMX178MIPI_Sensor_Init  */
static void VideoFullSizeSetting(void)//16:9   6M
{	//77 capture setting
	SENSORDB("[IMX178MIPI]enter VideoFullSizeSetting function\n");
	IMX178MIPI_write_cmos_sensor(0x41C0, 0x01); 		
	IMX178MIPI_write_cmos_sensor(0x0100, 0x00); 		 
//	IMX178MIPI_write_cmos_sensor(0x0101, 0x00); 		
	IMX178MIPI_write_cmos_sensor(0x030E, 0x0C); 		 
	IMX178MIPI_write_cmos_sensor(0x0202, 0x06);
	IMX178MIPI_write_cmos_sensor(0x0203, 0x8A);
	//PLLsetting
	IMX178MIPI_write_cmos_sensor(0x0301, 0x0A); 		 
	IMX178MIPI_write_cmos_sensor(0x0303, 0x01); 		 
	IMX178MIPI_write_cmos_sensor(0x0305, 0x06); 		 
	IMX178MIPI_write_cmos_sensor(0x0309, 0x0A); 		 
	IMX178MIPI_write_cmos_sensor(0x030B, 0x01); 		 
	IMX178MIPI_write_cmos_sensor(0x030C, 0x00);
	IMX178MIPI_write_cmos_sensor(0x030D, 0xDD);
	IMX178MIPI_write_cmos_sensor(0x030E, 0x01); 
	
	IMX178MIPI_write_cmos_sensor(0x0340, 0x06);
	IMX178MIPI_write_cmos_sensor(0x0341, 0x8E); 		 
	IMX178MIPI_write_cmos_sensor(0x0342, 0x0D); 		 
	IMX178MIPI_write_cmos_sensor(0x0343, 0x70); 		 
	IMX178MIPI_write_cmos_sensor(0x0344, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0345, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0346, 0x01); 		 
	IMX178MIPI_write_cmos_sensor(0x0347, 0x90); 		 
	IMX178MIPI_write_cmos_sensor(0x0348, 0x0C); 		 
	IMX178MIPI_write_cmos_sensor(0x0349, 0xCF); 		 
	IMX178MIPI_write_cmos_sensor(0x034A, 0x08); 		 
	IMX178MIPI_write_cmos_sensor(0x034B, 0x0F); 		 
	IMX178MIPI_write_cmos_sensor(0x034C, 0x0C); 		 
	IMX178MIPI_write_cmos_sensor(0x034D, 0xD0); 		 
	IMX178MIPI_write_cmos_sensor(0x034E, 0x06); 		
	IMX178MIPI_write_cmos_sensor(0x034F, 0x80); 		 
	IMX178MIPI_write_cmos_sensor(0x0383, 0x01); 		 
	IMX178MIPI_write_cmos_sensor(0x0387, 0x01); 		 
	IMX178MIPI_write_cmos_sensor(0x0390, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0401, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0405, 0x10); 		 
	IMX178MIPI_write_cmos_sensor(0x3020, 0x10); 		 
	IMX178MIPI_write_cmos_sensor(0x3041, 0x15); 		 
	IMX178MIPI_write_cmos_sensor(0x3042, 0x87); 		 
	IMX178MIPI_write_cmos_sensor(0x3089, 0x4F); 		 
	IMX178MIPI_write_cmos_sensor(0x3309, 0x9A); 		 
	IMX178MIPI_write_cmos_sensor(0x3344, 0x6F); 		 
	IMX178MIPI_write_cmos_sensor(0x3345, 0x1F); 		 
	IMX178MIPI_write_cmos_sensor(0x3362, 0x0A); 		 
	IMX178MIPI_write_cmos_sensor(0x3363, 0x0A); 		 
	IMX178MIPI_write_cmos_sensor(0x3364, 0x02); 		 
	IMX178MIPI_write_cmos_sensor(0x3368, 0x18); 		 
	IMX178MIPI_write_cmos_sensor(0x3369, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x3370, 0x7F); 		
	IMX178MIPI_write_cmos_sensor(0x3371, 0x37); 		 
	IMX178MIPI_write_cmos_sensor(0x3372, 0x67); 		 
	IMX178MIPI_write_cmos_sensor(0x3373, 0x3F); 		 
	IMX178MIPI_write_cmos_sensor(0x3374, 0x3F); 		
	IMX178MIPI_write_cmos_sensor(0x3375, 0x47); 		 
	IMX178MIPI_write_cmos_sensor(0x3376, 0xCF); 		 
	IMX178MIPI_write_cmos_sensor(0x3377, 0x47); 		 
	IMX178MIPI_write_cmos_sensor(0x33C8, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x33D4, 0x0C); 		 
	IMX178MIPI_write_cmos_sensor(0x33D5, 0xD0); 		 
	IMX178MIPI_write_cmos_sensor(0x33D6, 0x06); 		 
	IMX178MIPI_write_cmos_sensor(0x33D7, 0x80); 		 
	IMX178MIPI_write_cmos_sensor(0x4100, 0x0E); 		 
	IMX178MIPI_write_cmos_sensor(0x4108, 0x01); 		 
	IMX178MIPI_write_cmos_sensor(0x4109, 0x7C);  

#ifdef IMX178_USE_OTP
#ifdef IMX178_USE_AWB_OTP
	imx178_update_awb_gain();
#endif
#endif

	IMX178MIPI_write_cmos_sensor(0x0100, 0x01);//STREAM ON
	SENSORDB("[IMX178MIPI]exit VideoFullSizeSetting function\n"); 
}

static void PreviewSetting(void)
{	//77 capture setting
	SENSORDB("[IMX178MIPI]enter PreviewSetting function\n");
	IMX178MIPI_write_cmos_sensor(0x41C0, 0x01);
	IMX178MIPI_write_cmos_sensor(0x0104, 0x01);//group
	IMX178MIPI_write_cmos_sensor(0x0100, 0x00);//STREAM OFF 	
	IMX178MIPI_write_cmos_sensor(0x0103, 0x01);//SW reset
//	IMX178MIPI_write_cmos_sensor(0x0101, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x030E, 0x01); 	   
	IMX178MIPI_write_cmos_sensor(0x0202, 0x06); 		 
	IMX178MIPI_write_cmos_sensor(0x0203, 0xAD);
	//PLL setting
	IMX178MIPI_write_cmos_sensor(0x0301, 0x0A); 		 
	IMX178MIPI_write_cmos_sensor(0x0303, 0x01); 		 
	IMX178MIPI_write_cmos_sensor(0x0305, 0x06); 		 
	IMX178MIPI_write_cmos_sensor(0x0309, 0x0A); 		 
	IMX178MIPI_write_cmos_sensor(0x030B, 0x01); 		 
	IMX178MIPI_write_cmos_sensor(0x030C, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x030D, 0xDD); 
	IMX178MIPI_write_cmos_sensor(0x030E, 0x01);

	IMX178MIPI_write_cmos_sensor(0x0340, 0x06);
	IMX178MIPI_write_cmos_sensor(0x0341, 0xB1); 		 
	IMX178MIPI_write_cmos_sensor(0x0342, 0x0D); 		
	IMX178MIPI_write_cmos_sensor(0x0343, 0x70); 		 
	IMX178MIPI_write_cmos_sensor(0x0344, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0345, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0346, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0347, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0348, 0x0C); 		
	IMX178MIPI_write_cmos_sensor(0x0349, 0xCF); 		 
	IMX178MIPI_write_cmos_sensor(0x034A, 0x09); 		 
	IMX178MIPI_write_cmos_sensor(0x034B, 0x9F); 		 
	IMX178MIPI_write_cmos_sensor(0x034C, 0x06); 		 
	IMX178MIPI_write_cmos_sensor(0x034D, 0x68); 		 
	IMX178MIPI_write_cmos_sensor(0x034E, 0x04); 		 
	IMX178MIPI_write_cmos_sensor(0x034F, 0xD0); 		 
	IMX178MIPI_write_cmos_sensor(0x0383, 0x01); 		 
	IMX178MIPI_write_cmos_sensor(0x0387, 0x01); 		
	IMX178MIPI_write_cmos_sensor(0x0390, 0x01); 		 
	IMX178MIPI_write_cmos_sensor(0x0401, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0405, 0x10); 		 
	IMX178MIPI_write_cmos_sensor(0x3020, 0x10); 		 
	IMX178MIPI_write_cmos_sensor(0x3041, 0x15); 		 
	IMX178MIPI_write_cmos_sensor(0x3042, 0x87); 		 
	IMX178MIPI_write_cmos_sensor(0x3089, 0x4F); 		 
	IMX178MIPI_write_cmos_sensor(0x3309, 0x9A); 		 
	IMX178MIPI_write_cmos_sensor(0x3344, 0x6F); 		 
	IMX178MIPI_write_cmos_sensor(0x3345, 0x1F); 		 
	IMX178MIPI_write_cmos_sensor(0x3362, 0x0A); 		 
	IMX178MIPI_write_cmos_sensor(0x3363, 0x0A); 		 
	IMX178MIPI_write_cmos_sensor(0x3364, 0x02); 		
	IMX178MIPI_write_cmos_sensor(0x3368, 0x18); 		 
	IMX178MIPI_write_cmos_sensor(0x3369, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x3370, 0x7F); 		 
	IMX178MIPI_write_cmos_sensor(0x3371, 0x37); 		 
	IMX178MIPI_write_cmos_sensor(0x3372, 0x67); 		 
	IMX178MIPI_write_cmos_sensor(0x3373, 0x3F); 		 
	IMX178MIPI_write_cmos_sensor(0x3374, 0x3F); 		 
	IMX178MIPI_write_cmos_sensor(0x3375, 0x47); 		 
	IMX178MIPI_write_cmos_sensor(0x3376, 0xCF); 		 
	IMX178MIPI_write_cmos_sensor(0x3377, 0x47); 		
	IMX178MIPI_write_cmos_sensor(0x33C8, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x33D4, 0x06); 		 
	IMX178MIPI_write_cmos_sensor(0x33D5, 0x68); 		 
	IMX178MIPI_write_cmos_sensor(0x33D6, 0x04); 	   
	IMX178MIPI_write_cmos_sensor(0x33D7, 0xD0); 	  
	IMX178MIPI_write_cmos_sensor(0x4100, 0x0E); 	   
	IMX178MIPI_write_cmos_sensor(0x4108, 0x01);
	IMX178MIPI_write_cmos_sensor(0x4109, 0x7C);
	
#ifdef IMX178_USE_OTP
#ifdef IMX178_USE_AWB_OTP
	imx178_update_awb_gain();
#endif
#endif

	IMX178MIPI_write_cmos_sensor(0x0104, 0x00);//group
	IMX178MIPI_write_cmos_sensor(0x0100, 0x01);//STREAM ON
	spin_lock(&imx178_drv_lock);  
	IMX178MIPI_Auto_Flicker_mode = KAL_FALSE;	  // reset the flicker status	 
	spin_unlock(&imx178_drv_lock);
	SENSORDB("[IMX178MIPI]exit PreviewSetting function\n");
}


void IMX178MIPI_set_8M(void)
{	//77 capture setting
	SENSORDB("[IMX178MIPI]enter IMX178MIPI_set_8M function\n");
	IMX178MIPI_write_cmos_sensor(0x41C0, 0x01);         
	IMX178MIPI_write_cmos_sensor(0x0100, 0x00);          
//	IMX178MIPI_write_cmos_sensor(0x0101, 0x00);         
	IMX178MIPI_write_cmos_sensor(0x030E, 0x0C);          
	IMX178MIPI_write_cmos_sensor(0x0202, 0x09);
	IMX178MIPI_write_cmos_sensor(0x0203, 0xAA);
	//PLLsetting
	IMX178MIPI_write_cmos_sensor(0x0301, 0x0A);          
	IMX178MIPI_write_cmos_sensor(0x0303, 0x01);          
	IMX178MIPI_write_cmos_sensor(0x0305, 0x06);          
	IMX178MIPI_write_cmos_sensor(0x0309, 0x0A);          
	IMX178MIPI_write_cmos_sensor(0x030B, 0x01);          
	IMX178MIPI_write_cmos_sensor(0x030C, 0x00);
	IMX178MIPI_write_cmos_sensor(0x030D, 0xDD);
	IMX178MIPI_write_cmos_sensor(0x030E, 0x01); 
	
	IMX178MIPI_write_cmos_sensor(0x0340, 0x09);
	IMX178MIPI_write_cmos_sensor(0x0341, 0xAE);          
	IMX178MIPI_write_cmos_sensor(0x0342, 0x0D);          
	IMX178MIPI_write_cmos_sensor(0x0343, 0x70);          
	IMX178MIPI_write_cmos_sensor(0x0344, 0x00);          
	IMX178MIPI_write_cmos_sensor(0x0345, 0x00);          
	IMX178MIPI_write_cmos_sensor(0x0346, 0x00);          
	IMX178MIPI_write_cmos_sensor(0x0347, 0x00);          
	IMX178MIPI_write_cmos_sensor(0x0348, 0x0C);          
	IMX178MIPI_write_cmos_sensor(0x0349, 0xCF);          
	IMX178MIPI_write_cmos_sensor(0x034A, 0x09);          
	IMX178MIPI_write_cmos_sensor(0x034B, 0x9F);          
	IMX178MIPI_write_cmos_sensor(0x034C, 0x0C);          
	IMX178MIPI_write_cmos_sensor(0x034D, 0xD0);          
	IMX178MIPI_write_cmos_sensor(0x034E, 0x09);         
	IMX178MIPI_write_cmos_sensor(0x034F, 0xA0);          
	IMX178MIPI_write_cmos_sensor(0x0383, 0x01);          
	IMX178MIPI_write_cmos_sensor(0x0387, 0x01);          
	IMX178MIPI_write_cmos_sensor(0x0390, 0x00);          
	IMX178MIPI_write_cmos_sensor(0x0401, 0x00);          
	IMX178MIPI_write_cmos_sensor(0x0405, 0x10);          
	IMX178MIPI_write_cmos_sensor(0x3020, 0x10);          
	IMX178MIPI_write_cmos_sensor(0x3041, 0x15);          
	IMX178MIPI_write_cmos_sensor(0x3042, 0x87);          
	IMX178MIPI_write_cmos_sensor(0x3089, 0x4F);          
	IMX178MIPI_write_cmos_sensor(0x3309, 0x9A);          
	IMX178MIPI_write_cmos_sensor(0x3344, 0x6F);          
	IMX178MIPI_write_cmos_sensor(0x3345, 0x1F);          
	IMX178MIPI_write_cmos_sensor(0x3362, 0x0A);          
	IMX178MIPI_write_cmos_sensor(0x3363, 0x0A);          
	IMX178MIPI_write_cmos_sensor(0x3364, 0x02);          
	IMX178MIPI_write_cmos_sensor(0x3368, 0x18);          
	IMX178MIPI_write_cmos_sensor(0x3369, 0x00);          
	IMX178MIPI_write_cmos_sensor(0x3370, 0x7F);         
	IMX178MIPI_write_cmos_sensor(0x3371, 0x37);          
	IMX178MIPI_write_cmos_sensor(0x3372, 0x67);          
	IMX178MIPI_write_cmos_sensor(0x3373, 0x3F);          
	IMX178MIPI_write_cmos_sensor(0x3374, 0x3F);         
	IMX178MIPI_write_cmos_sensor(0x3375, 0x47);          
	IMX178MIPI_write_cmos_sensor(0x3376, 0xCF);          
	IMX178MIPI_write_cmos_sensor(0x3377, 0x47);          
	IMX178MIPI_write_cmos_sensor(0x33C8, 0x00);          
	IMX178MIPI_write_cmos_sensor(0x33D4, 0x0C);          
	IMX178MIPI_write_cmos_sensor(0x33D5, 0xD0);          
	IMX178MIPI_write_cmos_sensor(0x33D6, 0x09);          
	IMX178MIPI_write_cmos_sensor(0x33D7, 0xA0);          
	IMX178MIPI_write_cmos_sensor(0x4100, 0x0E);          
	IMX178MIPI_write_cmos_sensor(0x4108, 0x01);          
	IMX178MIPI_write_cmos_sensor(0x4109, 0x7C);  

#ifdef IMX178_USE_OTP
#ifdef IMX178_USE_AWB_OTP
	imx178_update_awb_gain();
#endif
#endif

	IMX178MIPI_write_cmos_sensor(0x0100, 0x01);//STREAM ON
	#if 0
	IMX178MIPI_write_cmos_sensor(0x41C0, 0x01);
	IMX178MIPI_write_cmos_sensor(0x0104, 0x01);//group
	IMX178MIPI_write_cmos_sensor(0x0100, 0x00);//STREAM OFF 	
	IMX178MIPI_write_cmos_sensor(0x0103, 0x01);//SW reset
	IMX178MIPI_write_cmos_sensor(0x0101, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x030E, 0x01); 	   
	IMX178MIPI_write_cmos_sensor(0x0202, 0x09); 		 
	IMX178MIPI_write_cmos_sensor(0x0203, 0xAA);
	//PLL setting
	IMX178MIPI_write_cmos_sensor(0x0301, 0x0A); 		 
	IMX178MIPI_write_cmos_sensor(0x0303, 0x01); 		 
	IMX178MIPI_write_cmos_sensor(0x0305, 0x06); 		 
	IMX178MIPI_write_cmos_sensor(0x0309, 0x0A); 		 
	IMX178MIPI_write_cmos_sensor(0x030B, 0x01); 		 
	IMX178MIPI_write_cmos_sensor(0x030C, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x030D, 0xDD); 
	IMX178MIPI_write_cmos_sensor(0x030E, 0x01);

	IMX178MIPI_write_cmos_sensor(0x0340, 0x09);
	IMX178MIPI_write_cmos_sensor(0x0341, 0xAE); 		 
	IMX178MIPI_write_cmos_sensor(0x0342, 0x0D); 		
	IMX178MIPI_write_cmos_sensor(0x0343, 0x70); 		 
	IMX178MIPI_write_cmos_sensor(0x0344, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0345, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0346, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0347, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0348, 0x0C); 		
	IMX178MIPI_write_cmos_sensor(0x0349, 0xCF); 		 
	IMX178MIPI_write_cmos_sensor(0x034A, 0x09); 		 
	IMX178MIPI_write_cmos_sensor(0x034B, 0x9F); 		 
	IMX178MIPI_write_cmos_sensor(0x034C, 0x0C); 		 
	IMX178MIPI_write_cmos_sensor(0x034D, 0xD0); 		 
	IMX178MIPI_write_cmos_sensor(0x034E, 0x09); 		 
	IMX178MIPI_write_cmos_sensor(0x034F, 0xA0); 		 
	IMX178MIPI_write_cmos_sensor(0x0383, 0x01); 		 
	IMX178MIPI_write_cmos_sensor(0x0387, 0x01); 		
	IMX178MIPI_write_cmos_sensor(0x0390, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0401, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x0405, 0x10); 		 
	IMX178MIPI_write_cmos_sensor(0x3020, 0x10); 		 
	IMX178MIPI_write_cmos_sensor(0x3041, 0x15); 		 
	IMX178MIPI_write_cmos_sensor(0x3042, 0x87); 		 
	IMX178MIPI_write_cmos_sensor(0x3089, 0x4F); 		 
	IMX178MIPI_write_cmos_sensor(0x3309, 0x9A); 		 
	IMX178MIPI_write_cmos_sensor(0x3344, 0x6F); 		 
	IMX178MIPI_write_cmos_sensor(0x3345, 0x1F); 		 
	IMX178MIPI_write_cmos_sensor(0x3362, 0x0A); 		 
	IMX178MIPI_write_cmos_sensor(0x3363, 0x0A); 		 
	IMX178MIPI_write_cmos_sensor(0x3364, 0x02); 		
	IMX178MIPI_write_cmos_sensor(0x3368, 0x18); 		 
	IMX178MIPI_write_cmos_sensor(0x3369, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x3370, 0x7F); 		 
	IMX178MIPI_write_cmos_sensor(0x3371, 0x37); 		 
	IMX178MIPI_write_cmos_sensor(0x3372, 0x67); 		 
	IMX178MIPI_write_cmos_sensor(0x3373, 0x3F); 		 
	IMX178MIPI_write_cmos_sensor(0x3374, 0x3F); 		 
	IMX178MIPI_write_cmos_sensor(0x3375, 0x47); 		 
	IMX178MIPI_write_cmos_sensor(0x3376, 0xCF); 		 
	IMX178MIPI_write_cmos_sensor(0x3377, 0x47); 		
	IMX178MIPI_write_cmos_sensor(0x33C8, 0x00); 		 
	IMX178MIPI_write_cmos_sensor(0x33D4, 0x0C); 		 
	IMX178MIPI_write_cmos_sensor(0x33D5, 0xD0); 		 
	IMX178MIPI_write_cmos_sensor(0x33D6, 0x09); 	   
	IMX178MIPI_write_cmos_sensor(0x33D7, 0xA0); 	  
	IMX178MIPI_write_cmos_sensor(0x4100, 0x0E); 	   
	IMX178MIPI_write_cmos_sensor(0x4108, 0x01);
	IMX178MIPI_write_cmos_sensor(0x4109, 0x7C);
	IMX178MIPI_write_cmos_sensor(0x0104, 0x00);//group
	IMX178MIPI_write_cmos_sensor(0x0100, 0x01);//STREAM ON
	#endif
	SENSORDB("[IMX178MIPI]exit IMX178MIPI_set_8M function\n"); 
}
/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*   IMX178MIPIOpen
*
* DESCRIPTION
*   This function initialize the registers of CMOS sensor
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

UINT32 IMX178MIPIOpen(void)
{
#ifdef IMX178_USE_OTP
    if(0 == used_otp){
	printk("before otp............................................\n");
	printk("before otp............................................\n");
	printk("before otp............................................\n");


	#ifdef IMX178_USE_AWB_OTP
	imx178_update_otp_wb();
	#endif

	used_otp =1;
	printk("after otp............................................\n");
	printk("after otp............................................\n");
	printk("after otp............................................\n");
    }
#endif

    int  retry = 0; 
	kal_uint16 sensorid;
    // check if sensor ID correct
    retry = 3; 
	SENSORDB("[IMX178MIPI]enter IMX178MIPIOpen function\n");
    do {
       SENSORDB("Read ID in the Open function"); 
	   sensorid=(kal_uint16)(((IMX178MIPI_read_cmos_sensor(0x0002)&&0x0f)<<8) | IMX178MIPI_read_cmos_sensor(0x0003));  
	   spin_lock(&imx178_drv_lock);    
	   if(sensorid == IMX179_SENSOR_ID)
	   	IMX178MIPI_sensor_id =IMX178MIPI_SENSOR_ID;
	   spin_unlock(&imx178_drv_lock);
		if (IMX178MIPI_sensor_id == IMX178MIPI_SENSOR_ID)
		break; 
		SENSORDB("Read Sensor ID Fail = 0x%04x\n", IMX178MIPI_sensor_id); 
		retry--; 
	    }
	while (retry > 0);
    SENSORDB("Read Sensor ID = 0x%04x\n", IMX178MIPI_sensor_id);
    if (IMX178MIPI_sensor_id != IMX178MIPI_SENSOR_ID)
        return ERROR_SENSOR_CONNECT_FAIL;
    IMX178MIPI_Sensor_Init();
	sensorid=read_IMX178MIPI_gain();
	spin_lock(&imx178_drv_lock);	
    IMX178MIPI_sensor_gain_base = sensorid;
	spin_unlock(&imx178_drv_lock);
	SENSORDB("[IMX178MIPI]exit IMX178MIPIOpen function\n");
    return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*   IMX178MIPIGetSensorID
*
* DESCRIPTION
*   This function get the sensor ID 
*
* PARAMETERS
*   *sensorID : return the sensor ID 
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 IMX178MIPIGetSensorID(UINT32 *sensorID) 
{
    int  retry = 3; 
    UINT32 sensor_id;
    int customer_otp_id;
	SENSORDB("[IMX178MIPI]enter IMX178MIPIGetSensorID function\n");
    // check if sensor ID correct
    do {		
	   sensor_id =(kal_uint16)(((IMX178MIPI_read_cmos_sensor(0x0002)&&0x0f)<<8) | IMX178MIPI_read_cmos_sensor(0x0003)); 

	  if (sensor_id == IMX179_SENSOR_ID)  
        {
	     customer_otp_id =get_sensor_imx178_customer_id();			 //customer_otp_id 			shunyu:1  truly:2
	     if(customer_otp_id == 5){
	            *sensorID = IMX178MIPI_SENSOR_ID;
	            break;
	     }
        }
			
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", *sensorID); 
        retry--; 
    } while (retry > 0);

    if (*sensorID != IMX178MIPI_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
	SENSORDB("[IMX178MIPI]exit IMX178MIPIGetSensorID function\n");
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*   IMX178MIPI_SetShutter
*
* DESCRIPTION
*   This function set e-shutter of IMX178MIPI to change exposure time.
*
* PARAMETERS
*   shutter : exposured lines
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void IMX178MIPI_SetShutter(kal_uint16 iShutter)
{

	SENSORDB("[IMX178MIPI]%s():shutter=%d\n",__FUNCTION__,iShutter);
   
   SENSORDB("shutter&gain test by hhl: set shutter IMX178MIPI_SetShutter\n"); 
    if (iShutter < 1)
        iShutter = 1; 
	else if(iShutter > 0xffff)
		iShutter = 0xffff;
	unsigned long flags;
	spin_lock_irqsave(&imx178_drv_lock,flags);
    IMX178MIPI_sensor.pv_shutter = iShutter;	
	spin_unlock_irqrestore(&imx178_drv_lock,flags);
    IMX178MIPI_write_shutter(iShutter);
	SENSORDB("[IMX178MIPI]exit IMX178MIPIGetSensorID function\n");
}   /*  IMX178MIPI_SetShutter   */



/*************************************************************************
* FUNCTION
*   IMX178MIPI_read_shutter
*
* DESCRIPTION
*   This function to  Get exposure time.
*
* PARAMETERS
*   None
*
* RETURNS
*   shutter : exposured lines
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT16 IMX178MIPI_read_shutter(void)
{
    return (UINT16)( (IMX178MIPI_read_cmos_sensor(0x0202)<<8) | IMX178MIPI_read_cmos_sensor(0x0203) );
}

/*************************************************************************
* FUNCTION
*   IMX178MIPI_night_mode
*
* DESCRIPTION
*   This function night mode of IMX178MIPI.
*
* PARAMETERS
*   none
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void IMX178MIPI_NightMode(kal_bool bEnable)
{
	SENSORDB("[IMX178MIPI]enter IMX178MIPI_NightMode function\n");
#if 0
    /************************************************************************/
    /*                      Auto Mode: 30fps                                                                                          */
    /*                      Night Mode:15fps                                                                                          */
    /************************************************************************/
    if(bEnable)
    {
        if(OV5642_MPEG4_encode_mode==KAL_TRUE)
        {
            OV5642_MAX_EXPOSURE_LINES = (kal_uint16)((OV5642_sensor_pclk/15)/(OV5642_PV_PERIOD_PIXEL_NUMS+OV5642_PV_dummy_pixels));
            OV5642_write_cmos_sensor(0x350C, (OV5642_MAX_EXPOSURE_LINES >> 8) & 0xFF);
            OV5642_write_cmos_sensor(0x350D, OV5642_MAX_EXPOSURE_LINES & 0xFF);
            OV5642_CURRENT_FRAME_LINES = OV5642_MAX_EXPOSURE_LINES;
            OV5642_MAX_EXPOSURE_LINES = OV5642_CURRENT_FRAME_LINES - OV5642_SHUTTER_LINES_GAP;
        }
    }
    else// Fix video framerate 30 fps
    {
        if(OV5642_MPEG4_encode_mode==KAL_TRUE)
        {
            OV5642_MAX_EXPOSURE_LINES = (kal_uint16)((OV5642_sensor_pclk/30)/(OV5642_PV_PERIOD_PIXEL_NUMS+OV5642_PV_dummy_pixels));
            if(OV5642_pv_exposure_lines < (OV5642_MAX_EXPOSURE_LINES - OV5642_SHUTTER_LINES_GAP)) // for avoid the shutter > frame_lines,move the frame lines setting to shutter function
            {
                OV5642_write_cmos_sensor(0x350C, (OV5642_MAX_EXPOSURE_LINES >> 8) & 0xFF);
                OV5642_write_cmos_sensor(0x350D, OV5642_MAX_EXPOSURE_LINES & 0xFF);
                OV5642_CURRENT_FRAME_LINES = OV5642_MAX_EXPOSURE_LINES;
            }
            OV5642_MAX_EXPOSURE_LINES = OV5642_MAX_EXPOSURE_LINES - OV5642_SHUTTER_LINES_GAP;
        }
    }
	
#endif	
	SENSORDB("[IMX178MIPI]exit IMX178MIPI_NightMode function\n");
}/*	IMX178MIPI_NightMode */



/*************************************************************************
* FUNCTION
*   IMX178MIPIClose
*
* DESCRIPTION
*   This function is to turn off sensor module power.
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
UINT32 IMX178MIPIClose(void)
{
    IMX178MIPI_write_cmos_sensor(0x0100,0x00);
    return ERROR_NONE;
}	/* IMX178MIPIClose() */

void IMX178MIPISetFlipMirror(kal_int32 imgMirror)
{
    kal_uint8  iTemp; 
	SENSORDB("[IMX178MIPI]enter IMX178MIPISetFlipMirror function,imgMirror=%d\n",imgMirror);
    iTemp = IMX178MIPI_read_cmos_sensor(0x0101) & 0x03;	//Clear the mirror and flip bits.
    switch (imgMirror)
    {
        case IMAGE_NORMAL:
            IMX178MIPI_write_cmos_sensor(0x0101, 0x03);	//Set normal
            break;
        case IMAGE_V_MIRROR:
            IMX178MIPI_write_cmos_sensor(0x0101, iTemp | 0x01);	//Set flip
            break;
        case IMAGE_H_MIRROR:
            IMX178MIPI_write_cmos_sensor(0x0101, iTemp | 0x02);	//Set mirror
            break;
        case IMAGE_HV_MIRROR:
            IMX178MIPI_write_cmos_sensor(0x0101, 0x00);	//Set mirror and flip
            break;
    }
	SENSORDB("[IMX178MIPI]exit IMX178MIPISetFlipMirror function\n");
}


/*************************************************************************
* FUNCTION
*   IMX178MIPIPreview
*
* DESCRIPTION
*   This function start the sensor preview.
*
* PARAMETERS
*   *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 IMX178MIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint16 iStartX = 0, iStartY = 0;	
	SENSORDB("[IMX178MIPI]enter IMX178MIPIPreview function\n");
	spin_lock(&imx178_drv_lock);    
	IMX178MIPI_MPEG4_encode_mode = KAL_FALSE;
	IMX178MIPI_sensor.video_mode=KAL_FALSE;
	IMX178MIPI_sensor.pv_mode=KAL_TRUE;
	IMX178MIPI_sensor.capture_mode=KAL_FALSE;
	spin_unlock(&imx178_drv_lock);
	PreviewSetting();
	iStartX += IMX178MIPI_IMAGE_SENSOR_PV_STARTX;
	iStartY += IMX178MIPI_IMAGE_SENSOR_PV_STARTY;
	spin_lock(&imx178_drv_lock);	
	IMX178MIPI_sensor.cp_dummy_pixels = 0;
	IMX178MIPI_sensor.cp_dummy_lines = 0;
	IMX178MIPI_sensor.pv_dummy_pixels = 0;
	IMX178MIPI_sensor.pv_dummy_lines = 0;
	IMX178MIPI_sensor.video_dummy_pixels = 0;
	IMX178MIPI_sensor.video_dummy_lines = 0;
	IMX178MIPI_sensor.pv_line_length = IMX178MIPI_PV_LINE_LENGTH_PIXELS+IMX178MIPI_sensor.pv_dummy_pixels; 
	IMX178MIPI_sensor.pv_frame_length = IMX178MIPI_PV_FRAME_LENGTH_LINES+IMX178MIPI_sensor.pv_dummy_lines;
	spin_unlock(&imx178_drv_lock);

	IMX178MIPI_SetDummy(IMX178MIPI_sensor.pv_dummy_pixels,IMX178MIPI_sensor.pv_dummy_lines);
	IMX178MIPI_SetShutter(IMX178MIPI_sensor.pv_shutter);
	spin_lock(&imx178_drv_lock);	
	memcpy(&IMX178MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	spin_unlock(&imx178_drv_lock);
	image_window->GrabStartX= iStartX;
	image_window->GrabStartY= iStartY;
	image_window->ExposureWindowWidth= IMX178MIPI_REAL_PV_WIDTH ;
	image_window->ExposureWindowHeight= IMX178MIPI_REAL_PV_HEIGHT ;
	SENSORDB("Preview resolution:%d %d %d %d\n", image_window->GrabStartX, image_window->GrabStartY, image_window->ExposureWindowWidth, image_window->ExposureWindowHeight); 
	SENSORDB("[IMX178MIPI]eXIT IMX178MIPIPreview function\n"); 

	IMX178MIPISetFlipMirror(IMAGE_NORMAL);
	return ERROR_NONE;
	}	/* IMX178MIPIPreview() */

/*************************************************************************
* FUNCTION
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 IMX178MIPIVideo(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint16 iStartX = 0, iStartY = 0;
	SENSORDB("[IMX178MIPI]enter IMX178MIPIVideo function\n"); 
	spin_lock(&imx178_drv_lock);    
    IMX178MIPI_MPEG4_encode_mode = KAL_TRUE;  
	IMX178MIPI_sensor.video_mode=KAL_TRUE;
	IMX178MIPI_sensor.pv_mode=KAL_FALSE;
	IMX178MIPI_sensor.capture_mode=KAL_FALSE;
	spin_unlock(&imx178_drv_lock);
	VideoFullSizeSetting();
	iStartX += IMX178MIPI_IMAGE_SENSOR_VIDEO_STARTX;
	iStartY += IMX178MIPI_IMAGE_SENSOR_VIDEO_STARTY;
	spin_lock(&imx178_drv_lock);	
	IMX178MIPI_sensor.cp_dummy_pixels = 0;
	IMX178MIPI_sensor.cp_dummy_lines = 0;
	IMX178MIPI_sensor.pv_dummy_pixels = 0;
	IMX178MIPI_sensor.pv_dummy_lines = 0;
	IMX178MIPI_sensor.video_dummy_pixels = 0;
	IMX178MIPI_sensor.video_dummy_lines = 0;
	IMX178MIPI_sensor.video_line_length = IMX178MIPI_VIDEO_LINE_LENGTH_PIXELS+IMX178MIPI_sensor.video_dummy_pixels; 
	IMX178MIPI_sensor.video_frame_length = IMX178MIPI_VIDEO_FRAME_LENGTH_LINES+IMX178MIPI_sensor.video_dummy_lines;
	spin_unlock(&imx178_drv_lock);
	
	IMX178MIPI_SetDummy(IMX178MIPI_sensor.video_dummy_pixels,IMX178MIPI_sensor.video_dummy_lines);
	IMX178MIPI_SetShutter(IMX178MIPI_sensor.video_shutter);
	spin_lock(&imx178_drv_lock);	
	memcpy(&IMX178MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	spin_unlock(&imx178_drv_lock);
	image_window->GrabStartX= iStartX;
	image_window->GrabStartY= iStartY;
    SENSORDB("[IMX178MIPI]eXIT IMX178MIPIVideo function\n"); 
	IMX178MIPISetFlipMirror(IMAGE_NORMAL);
return ERROR_NONE;
}	/* IMX178MIPIPreview() */

UINT32 IMX178MIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint16 iStartX = 0, iStartY = 0;
	SENSORDB("[IMX178MIPI]enter IMX178MIPICapture function\n");
	spin_lock(&imx178_drv_lock);	
	IMX178MIPI_sensor.video_mode=KAL_FALSE;
	IMX178MIPI_sensor.pv_mode=KAL_FALSE;
	IMX178MIPI_sensor.capture_mode=KAL_TRUE;
	IMX178MIPI_MPEG4_encode_mode = KAL_FALSE; 
	IMX178MIPI_Auto_Flicker_mode = KAL_FALSE;       
		IMX178MIPI_sensor.cp_dummy_pixels = 0;
		IMX178MIPI_sensor.cp_dummy_lines = 0;
		spin_unlock(&imx178_drv_lock);
        IMX178MIPI_set_8M();
     
      //  IMX178MIPISetFlipMirror(sensor_config_data->SensorImageMirror); 
	 spin_lock(&imx178_drv_lock);    
     IMX178MIPI_sensor.cp_line_length=IMX178MIPI_FULL_LINE_LENGTH_PIXELS+IMX178MIPI_sensor.cp_dummy_pixels;
     IMX178MIPI_sensor.cp_frame_length=IMX178MIPI_FULL_FRAME_LENGTH_LINES+IMX178MIPI_sensor.cp_dummy_lines;
	 spin_unlock(&imx178_drv_lock);
	iStartX = IMX178MIPI_IMAGE_SENSOR_CAP_STARTX;
	iStartY = IMX178MIPI_IMAGE_SENSOR_CAP_STARTY;
	image_window->GrabStartX=iStartX;
	image_window->GrabStartY=iStartY;
	image_window->ExposureWindowWidth=IMX178MIPI_REAL_CAP_WIDTH ;
	image_window->ExposureWindowHeight=IMX178MIPI_REAL_CAP_HEIGHT;
    IMX178MIPI_SetDummy(IMX178MIPI_sensor.cp_dummy_pixels, IMX178MIPI_sensor.cp_dummy_lines);
	spin_lock(&imx178_drv_lock);	
    memcpy(&IMX178MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	spin_unlock(&imx178_drv_lock);
    SENSORDB("[IMX178MIPI]exit IMX178MIPICapture function\n");
    IMX178MIPISetFlipMirror(IMAGE_NORMAL);
    return ERROR_NONE;
}	/* IMX178MIPICapture() */

UINT32 IMX178MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    SENSORDB("[IMX178MIPI]eXIT IMX178MIPIGetResolution function\n");
    pSensorResolution->SensorPreviewWidth	= IMX178MIPI_REAL_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight	= IMX178MIPI_REAL_PV_HEIGHT;
    pSensorResolution->SensorFullWidth		= IMX178MIPI_REAL_CAP_WIDTH;
    pSensorResolution->SensorFullHeight		= IMX178MIPI_REAL_CAP_HEIGHT;
    pSensorResolution->SensorVideoWidth		= IMX178MIPI_REAL_VIDEO_WIDTH;
    pSensorResolution->SensorVideoHeight    = IMX178MIPI_REAL_VIDEO_HEIGHT;
    SENSORDB("IMX178MIPIGetResolution :8-14");    

    return ERROR_NONE;
}   /* IMX178MIPIGetResolution() */

UINT32 IMX178MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{ 
	SENSORDB("[IMX178MIPI]enter IMX178MIPIGetInfo function\n");
	switch(ScenarioId){
			case MSDK_SCENARIO_ID_CAMERA_ZSD:
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG://hhl 2-28
				pSensorInfo->SensorFullResolutionX=IMX178MIPI_REAL_CAP_WIDTH;
				pSensorInfo->SensorFullResolutionY=IMX178MIPI_REAL_CAP_HEIGHT;
				pSensorInfo->SensorStillCaptureFrameRate=22;

			break;//hhl 2-28
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				pSensorInfo->SensorPreviewResolutionX=IMX178MIPI_REAL_VIDEO_WIDTH;
				pSensorInfo->SensorPreviewResolutionY=IMX178MIPI_REAL_VIDEO_HEIGHT;
				pSensorInfo->SensorCameraPreviewFrameRate=30;
			break;
		default:
        pSensorInfo->SensorPreviewResolutionX=IMX178MIPI_REAL_PV_WIDTH;
        pSensorInfo->SensorPreviewResolutionY=IMX178MIPI_REAL_PV_HEIGHT;
				pSensorInfo->SensorCameraPreviewFrameRate=30;
			break;
	}
    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=24;
    pSensorInfo->SensorWebCamCaptureFrameRate=24;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=5;
  //  pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_R;
    pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_B;
    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW; /*??? */
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 1;
    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_MIPI;

    pSensorInfo->CaptureDelayFrame = 2; 
    pSensorInfo->PreviewDelayFrame = 2; 
    pSensorInfo->VideoDelayFrame = 2; 
    pSensorInfo->SensorMasterClockSwitch = 0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;      
    pSensorInfo->AEShutDelayFrame = 0;		    /* The frame of setting shutter default 0 for TG int */
    pSensorInfo->AESensorGainDelayFrame = 0;     /* The frame of setting sensor gain */
    pSensorInfo->AEISPGainDelayFrame = 2;	
	   
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=	5;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = IMX178MIPI_IMAGE_SENSOR_PV_STARTX; 
            pSensorInfo->SensorGrabStartY = IMX178MIPI_IMAGE_SENSOR_PV_STARTY;           		
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	     pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	     pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
		
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			   pSensorInfo->SensorClockFreq=24;
			   pSensorInfo->SensorClockDividCount= 5;
			   pSensorInfo->SensorClockRisingCount= 0;
			   pSensorInfo->SensorClockFallingCount= 2;
			   pSensorInfo->SensorPixelClockCount= 3;
			   pSensorInfo->SensorDataLatchCount= 2;
			   pSensorInfo->SensorGrabStartX = IMX178MIPI_IMAGE_SENSOR_VIDEO_STARTX; 
			   pSensorInfo->SensorGrabStartY = IMX178MIPI_IMAGE_SENSOR_VIDEO_STARTY;				   
			   pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;		   
			   pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
			pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
			pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
			   pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
			   pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
			   pSensorInfo->SensorPacketECCOrder = 1;

			break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=	5;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = IMX178MIPI_IMAGE_SENSOR_CAP_STARTX;	//2*IMX178MIPI_IMAGE_SENSOR_PV_STARTX; 
            pSensorInfo->SensorGrabStartY = IMX178MIPI_IMAGE_SENSOR_CAP_STARTY;	//2*IMX178MIPI_IMAGE_SENSOR_PV_STARTY;          			
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0; 
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        default:
			 pSensorInfo->SensorClockFreq=24;
			 pSensorInfo->SensorClockDividCount= 5;
			 pSensorInfo->SensorClockRisingCount= 0;
			 pSensorInfo->SensorClockFallingCount= 2;
			 pSensorInfo->SensorPixelClockCount= 3;
			 pSensorInfo->SensorDataLatchCount= 2;
			 pSensorInfo->SensorGrabStartX = IMX178MIPI_IMAGE_SENSOR_PV_STARTX; 
			 pSensorInfo->SensorGrabStartY = IMX178MIPI_IMAGE_SENSOR_PV_STARTY; 				 
			 pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;		 
			 pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
		     pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
		  pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
			 pSensorInfo->SensorWidthSampling = 0;	// 0 is default 1x
			 pSensorInfo->SensorHightSampling = 0;	 // 0 is default 1x 
			 pSensorInfo->SensorPacketECCOrder = 1;

            break;
    }
	spin_lock(&imx178_drv_lock);	

    IMX178MIPIPixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &IMX178MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	spin_unlock(&imx178_drv_lock);
    SENSORDB("[IMX178MIPI]exit IMX178MIPIGetInfo function\n");
    return ERROR_NONE;
}   /* IMX178MIPIGetInfo() */


UINT32 IMX178MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{    
		spin_lock(&imx178_drv_lock);	
		CurrentScenarioId = ScenarioId;
		spin_unlock(&imx178_drv_lock);
		SENSORDB("[IMX178MIPI]enter IMX178MIPIControl function\n");
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            IMX178MIPIPreview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			IMX178MIPIVideo(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	    case MSDK_SCENARIO_ID_CAMERA_ZSD:
            IMX178MIPICapture(pImageWindow, pSensorConfigData);//hhl 2-28
            break;
        default:
            return ERROR_INVALID_SCENARIO_ID;
    }
	SENSORDB("[IMX178MIPI]exit IMX178MIPIControl function\n");
    return ERROR_NONE;
} /* IMX178MIPIControl() */

UINT32 IMX178MIPISetVideoMode(UINT16 u2FrameRate)
{
         SENSORDB("[IMX178MIPISetVideoMode] frame rate = %d\n", u2FrameRate);
		kal_uint16 IMX178MIPI_Video_Max_Expourse_Time = 0;
		SENSORDB("[IMX178MIPI]%s():fix_frame_rate=%d\n",__FUNCTION__,u2FrameRate);

		spin_lock(&imx178_drv_lock);
		IMX178MIPI_sensor.fix_video_fps = KAL_TRUE;
		spin_unlock(&imx178_drv_lock);
		u2FrameRate=u2FrameRate*10;//10*FPS
		SENSORDB("[IMX178MIPI][Enter Fix_fps func] IMX178MIPI_Fix_Video_Frame_Rate = %d\n", u2FrameRate/10);
	
		IMX178MIPI_Video_Max_Expourse_Time = (kal_uint16)((IMX178MIPI_sensor.video_pclk*10/u2FrameRate)/IMX178MIPI_sensor.video_line_length);
		
		if (IMX178MIPI_Video_Max_Expourse_Time > IMX178MIPI_VIDEO_FRAME_LENGTH_LINES/*IMX178MIPI_sensor.pv_frame_length*/) 
			{
				spin_lock(&imx178_drv_lock);    
				IMX178MIPI_sensor.video_frame_length = IMX178MIPI_Video_Max_Expourse_Time;
				IMX178MIPI_sensor.video_dummy_lines = IMX178MIPI_sensor.video_frame_length-IMX178MIPI_VIDEO_FRAME_LENGTH_LINES;
				spin_unlock(&imx178_drv_lock);
				SENSORDB("[IMX178MIPI]%s():frame_length=%d,dummy_lines=%d\n",__FUNCTION__,IMX178MIPI_sensor.video_frame_length,IMX178MIPI_sensor.video_dummy_lines);
				IMX178MIPI_SetDummy(IMX178MIPI_sensor.video_dummy_pixels,IMX178MIPI_sensor.video_dummy_lines);
			}
	spin_lock(&imx178_drv_lock);    
    IMX178MIPI_MPEG4_encode_mode = KAL_TRUE; 
	spin_unlock(&imx178_drv_lock);
	SENSORDB("[IMX178MIPI]exit IMX178MIPISetVideoMode function\n");
	return ERROR_NONE;
}

UINT32 IMX178MIPISetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{
	kal_uint32 pv_max_frame_rate_lines=0;

	if(IMX178MIPI_sensor.pv_mode==TRUE)
	pv_max_frame_rate_lines=IMX178MIPI_PV_FRAME_LENGTH_LINES;
	else
    pv_max_frame_rate_lines=IMX178MIPI_VIDEO_FRAME_LENGTH_LINES	;
    SENSORDB("[IMX178MIPISetAutoFlickerMode] frame rate(10base) = %d %d\n", bEnable, u2FrameRate);
    if(bEnable) 
	{   // enable auto flicker   
    	spin_lock(&imx178_drv_lock);    
        IMX178MIPI_Auto_Flicker_mode = KAL_TRUE; 
		spin_unlock(&imx178_drv_lock);
        if(IMX178MIPI_MPEG4_encode_mode == KAL_TRUE) 
		{ // in the video mode, reset the frame rate
            pv_max_frame_rate_lines = IMX178MIPI_MAX_EXPOSURE_LINES + (IMX178MIPI_MAX_EXPOSURE_LINES>>7);            
            IMX178MIPI_write_cmos_sensor(0x0104, 1);        
            IMX178MIPI_write_cmos_sensor(0x0340, (pv_max_frame_rate_lines >>8) & 0xFF);
            IMX178MIPI_write_cmos_sensor(0x0341, pv_max_frame_rate_lines & 0xFF);	
            IMX178MIPI_write_cmos_sensor(0x0104, 0);        	
        }
    } 
	else 
	{
    	spin_lock(&imx178_drv_lock);    
        IMX178MIPI_Auto_Flicker_mode = KAL_FALSE; 
		spin_unlock(&imx178_drv_lock);
        if(IMX178MIPI_MPEG4_encode_mode == KAL_TRUE) 
		{    // in the video mode, restore the frame rate
            IMX178MIPI_write_cmos_sensor(0x0104, 1);        
            IMX178MIPI_write_cmos_sensor(0x0340, (IMX178MIPI_MAX_EXPOSURE_LINES >>8) & 0xFF);
            IMX178MIPI_write_cmos_sensor(0x0341, IMX178MIPI_MAX_EXPOSURE_LINES & 0xFF);	
            IMX178MIPI_write_cmos_sensor(0x0104, 0);        	
        }
        printk("Disable Auto flicker\n");    
    }
    return ERROR_NONE;
}




UINT32 IMX178MIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) {
	kal_uint32 pclk;
	kal_int16 dummyLine;
	kal_uint16 lineLength,frameHeight;
	SENSORDB("IMX178MIPISetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);
	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			pclk = 176800000;
			lineLength = IMX178MIPI_PV_LINE_LENGTH_PIXELS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - IMX178MIPI_PV_FRAME_LENGTH_LINES;

			
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&imx178_drv_lock);	
			IMX178MIPI_sensor.pv_mode=TRUE;
			spin_unlock(&imx178_drv_lock);
			IMX178MIPI_SetDummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pclk = 176800000;
			lineLength = IMX178MIPI_VIDEO_LINE_LENGTH_PIXELS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - IMX178MIPI_VIDEO_FRAME_LENGTH_LINES;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&imx178_drv_lock);	
			IMX178MIPI_sensor.pv_mode=TRUE;
			spin_unlock(&imx178_drv_lock);
			IMX178MIPI_SetDummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:			
			pclk = 176800000;
			lineLength = IMX178MIPI_FULL_LINE_LENGTH_PIXELS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - IMX178MIPI_FULL_FRAME_LENGTH_LINES;
			if(dummyLine<0)
				dummyLine = 0;
			
			spin_lock(&imx178_drv_lock);	
			IMX178MIPI_sensor.pv_mode=FALSE;
			spin_unlock(&imx178_drv_lock);
			IMX178MIPI_SetDummy(0, dummyLine);			
			break;		
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
            break;
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
			break;
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
			break;		
		default:
			break;
	}
	SENSORDB("[IMX178MIPI]exit IMX178MIPISetMaxFramerateByScenario function\n");
	return ERROR_NONE;
}



UINT32 IMX178MIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
{

	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			 *pframeRate = 300;
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 *pframeRate = 220;
			break;		//hhl 2-28
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
			 *pframeRate = 300;
			break;		
		default:
			break;
	}

	return ERROR_NONE;
}




UINT32 IMX178MIPISetTestPatternMode(kal_bool bEnable)
{
    SENSORDB("[IMX178MIPISetTestPatternMode] Test pattern enable:%d\n", bEnable);
    
    if(bEnable) {   // enable color bar   
        IMX178MIPI_write_cmos_sensor(0x30D8, 0x10);  // color bar test pattern
        IMX178MIPI_write_cmos_sensor(0x0600, 0x00);  // color bar test pattern
        IMX178MIPI_write_cmos_sensor(0x0601, 0x02);  // color bar test pattern 
    } else {
        IMX178MIPI_write_cmos_sensor(0x30D8, 0x00);  // disable color bar test pattern
    }
    return ERROR_NONE;
}

UINT32 IMX178MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
                                                                UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    UINT32 SensorRegNumber;
    UINT32 i;
    PNVRAM_SENSOR_DATA_STRUCT pSensorDefaultData=(PNVRAM_SENSOR_DATA_STRUCT) pFeaturePara;
    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_GROUP_INFO_STRUCT *pSensorGroupInfo=(MSDK_SENSOR_GROUP_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_ITEM_INFO_STRUCT *pSensorItemInfo=(MSDK_SENSOR_ITEM_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_ENG_INFO_STRUCT	*pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;

    switch (FeatureId)
    {
        case SENSOR_FEATURE_GET_RESOLUTION:
            *pFeatureReturnPara16++=IMX178MIPI_REAL_CAP_WIDTH;
            *pFeatureReturnPara16=IMX178MIPI_REAL_CAP_HEIGHT;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:
        		switch(CurrentScenarioId)
        		{
        			case MSDK_SCENARIO_ID_CAMERA_ZSD:
        		    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
 		            *pFeatureReturnPara16++=IMX178MIPI_sensor.cp_line_length;  
 		            *pFeatureReturnPara16=IMX178MIPI_sensor.cp_frame_length;
		            SENSORDB("Sensor period:%d %d\n",IMX178MIPI_sensor.cp_line_length, IMX178MIPI_sensor.cp_frame_length); 
		            *pFeatureParaLen=4;        				
        				break;
        			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					*pFeatureReturnPara16++=IMX178MIPI_sensor.video_line_length;  
					*pFeatureReturnPara16=IMX178MIPI_sensor.video_frame_length;
					 SENSORDB("Sensor period:%d %d\n", IMX178MIPI_sensor.video_line_length, IMX178MIPI_sensor.video_frame_length); 
					 *pFeatureParaLen=4;
						break;
        			default:	
					*pFeatureReturnPara16++=IMX178MIPI_sensor.pv_line_length;  
					*pFeatureReturnPara16=IMX178MIPI_sensor.pv_frame_length;
		            SENSORDB("Sensor period:%d %d\n", IMX178MIPI_sensor.pv_line_length, IMX178MIPI_sensor.pv_frame_length); 
		            *pFeatureParaLen=4;
	            break;
          	}
          	break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        		switch(CurrentScenarioId)
        		{
        			case MSDK_SCENARIO_ID_CAMERA_ZSD:
        			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		            *pFeatureReturnPara32 = IMX178MIPI_sensor.cp_pclk; //134400000
		            *pFeatureParaLen=4;		         	
					
		            SENSORDB("Sensor CPCLK:%dn",IMX178MIPI_sensor.cp_pclk); 
		         		break; //hhl 2-28
					case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
						*pFeatureReturnPara32 = IMX178MIPI_sensor.video_pclk;//57600000; //199200000;
						*pFeatureParaLen=4;
						SENSORDB("Sensor videoCLK:%d\n",IMX178MIPI_sensor.video_pclk); 
						break;
		         		default:
		            *pFeatureReturnPara32 = IMX178MIPI_sensor.pv_pclk;//57600000; //199200000;
		            *pFeatureParaLen=4;
					SENSORDB("Sensor pvclk:%d\n",IMX178MIPI_sensor.pv_pclk); 
		            break;
		         }
		         break;
        case SENSOR_FEATURE_SET_ESHUTTER:
            IMX178MIPI_SetShutter(*pFeatureData16);
            break;
		case SENSOR_FEATURE_SET_SENSOR_SYNC:
			break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            IMX178MIPI_NightMode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:
           IMX178MIPI_SetGain((UINT16) *pFeatureData16);
            
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			spin_lock(&imx178_drv_lock);    
            IMX178MIPI_isp_master_clock=*pFeatureData32;
			spin_unlock(&imx178_drv_lock);
            break;
        case SENSOR_FEATURE_SET_REGISTER:
			IMX178MIPI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = IMX178MIPI_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)
            {
            	spin_lock(&imx178_drv_lock);    
                IMX178MIPISensorCCT[i].Addr=*pFeatureData32++;
                IMX178MIPISensorCCT[i].Para=*pFeatureData32++; 
				spin_unlock(&imx178_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=IMX178MIPISensorCCT[i].Addr;
                *pFeatureData32++=IMX178MIPISensorCCT[i].Para; 
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {	spin_lock(&imx178_drv_lock);    
                IMX178MIPISensorReg[i].Addr=*pFeatureData32++;
                IMX178MIPISensorReg[i].Para=*pFeatureData32++;
				spin_unlock(&imx178_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=IMX178MIPISensorReg[i].Addr;
                *pFeatureData32++=IMX178MIPISensorReg[i].Para;
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=IMX178MIPI_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, IMX178MIPISensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, IMX178MIPISensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &IMX178MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            IMX178MIPI_camera_para_to_sensor();
            break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            IMX178MIPI_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=IMX178MIPI_get_sensor_group_count();
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            IMX178MIPI_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            IMX178MIPI_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            IMX178MIPI_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_GET_ENG_INFO:
            pSensorEngInfo->SensorId = 129;
            pSensorEngInfo->SensorType = CMOS_SENSOR;
            pSensorEngInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_B;
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ENG_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
            // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
            // if EEPROM does not exist in camera module.
            *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
            *pFeatureParaLen=4;
            break;

        case SENSOR_FEATURE_INITIALIZE_AF:
            break;
        case SENSOR_FEATURE_CONSTANT_AF:
            break;
        case SENSOR_FEATURE_MOVE_FOCUS_LENS:
            break;
        case SENSOR_FEATURE_SET_VIDEO_MODE:
            IMX178MIPISetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            IMX178MIPIGetSensorID(pFeatureReturnPara32); 
            break;             
        case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
            IMX178MIPISetAutoFlickerMode((BOOL)*pFeatureData16, *(pFeatureData16+1));            
	        break;
        case SENSOR_FEATURE_SET_TEST_PATTERN:
            IMX178MIPISetTestPatternMode((BOOL)*pFeatureData16);        	
            break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			IMX178MIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			IMX178MIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
			break;
        default:
            break;
    }
    return ERROR_NONE;
}	/* IMX178MIPIFeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncIMX178MIPI=
{
    IMX178MIPIOpen,
    IMX178MIPIGetInfo,
    IMX178MIPIGetResolution,
    IMX178MIPIFeatureControl,
    IMX178MIPIControl,
    IMX178MIPIClose
};

UINT32 IMX178_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncIMX178MIPI;
    return ERROR_NONE;
}   /* SensorInit() */

