/*****************************************************************************
 *
 * Filename:
 * ---------
 *   sensor.c
 *
 * Project:
 * --------
 *  
 *
 * Description:
 * ------------
 *   Source code of Sensor driver
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
 * [GC2145MIPIYUV V1.0.0]
 * 8.17.2012 Leo.Lee
 * .First Release
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by GalaxyCoreinc. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
//#include <windows.h>
//#include <memory.h>
//#include <nkintr.h>
//#include <ceddk.h>
//#include <ceddk_exp.h>

//#include "kal_release.h"
//#include "i2c_exp.h"
//#include "gpio_exp.h"
//#include "msdk_exp.h"
//#include "msdk_sensor_exp.h"
//#include "msdk_isp_exp.h"
//#include "base_regs.h"
//#include "Sensor.h"
//#include "camera_sensor_para.h"
//#include "CameraCustomized.h"

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
//#include <mach/mt6516_pll.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "gc2145mipi_yuv_Sensor.h"
#include "gc2145mipi_yuv_Camera_Sensor_para.h"
#include "gc2145mipi_yuv_CameraCustomized.h"

#define GC2145MIPIYUV_DEBUG
#ifdef GC2145MIPIYUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

#define GC2145MIPI_2Lane

#define GC2145_TEST_PATTERN_CHECKSUM (0xd1c77d11)

//#define DEBUG_SENSOR_GC2145MIPI


#define  GC2145MIPI_SET_PAGE0    GC2145MIPI_write_cmos_sensor(0xfe,0x00)
#define  GC2145MIPI_SET_PAGE1    GC2145MIPI_write_cmos_sensor(0xfe,0x01)
#define  GC2145MIPI_SET_PAGE2    GC2145MIPI_write_cmos_sensor(0xfe,0x02)
#define  GC2145MIPI_SET_PAGE3    GC2145MIPI_write_cmos_sensor(0xfe,0x03)


extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
/*************************************************************************
* FUNCTION
*    GC2145MIPI_write_cmos_sensor
*
* DESCRIPTION
*    This function wirte data to CMOS sensor through I2C
*
* PARAMETERS
*    addr: the 16bit address of register
*    para: the 8bit value of register
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void GC2145MIPI_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
kal_uint8 out_buff[2];

    out_buff[0] = addr;
    out_buff[1] = para;

    iWriteRegI2C((u8*)out_buff , (u16)sizeof(out_buff), GC2145MIPI_WRITE_ID); 

#if (defined(__GC2145MIPI_DEBUG_TRACE__))
  if (sizeof(out_buff) != rt) printk("I2C write %x, %x error\n", addr, para);
#endif
}

/*************************************************************************
* FUNCTION
*    GC2145MIPI_read_cmos_sensor
*
* DESCRIPTION
*    This function read data from CMOS sensor through I2C.
*
* PARAMETERS
*    addr: the 16bit address of register
*
* RETURNS
*    8bit data read through I2C
*
* LOCAL AFFECTED
*
*************************************************************************/
static kal_uint8 GC2145MIPI_read_cmos_sensor(kal_uint8 addr)
{
  kal_uint8 in_buff[1] = {0xFF};
  kal_uint8 out_buff[1];
  
  out_buff[0] = addr;

    if (0 != iReadRegI2C((u8*)out_buff , (u16) sizeof(out_buff), (u8*)in_buff, (u16) sizeof(in_buff), GC2145MIPI_WRITE_ID)) {
        SENSORDB("ERROR: GC2145MIPI_read_cmos_sensor \n");
    }

#if (defined(__GC2145MIPI_DEBUG_TRACE__))
  if (size != rt) printk("I2C read %x error\n", addr);
#endif

  return in_buff[0];
}


#ifdef DEBUG_SENSOR_GC2145MIPI
#define gc2145mipi_OP_CODE_INI		0x00		/* Initial value. */
#define gc2145mipi_OP_CODE_REG		0x01		/* Register */
#define gc2145mipi_OP_CODE_DLY		0x02		/* Delay */
#define gc2145mipi_OP_CODE_END		0x03		/* End of initial setting. */
static kal_uint16 fromsd;

typedef struct
{
	u16 init_reg;
	u16 init_val;	/* Save the register value and delay tick */
	u8 op_code;		/* 0 - Initial value, 1 - Register, 2 - Delay, 3 - End of setting. */
} gc2145mipi_initial_set_struct;

gc2145mipi_initial_set_struct gc2145mipi_Init_Reg[5000];

static u32 strtol(const char *nptr, u8 base)
{

	printk("gc2145mipi___%s____\n",__func__); 

	u8 ret;
	if(!nptr || (base!=16 && base!=10 && base!=8))
	{
		printk("gc2145mipi %s(): NULL pointer input\n", __FUNCTION__);
		return -1;
	}
	for(ret=0; *nptr; nptr++)
	{
		if((base==16 && *nptr>='A' && *nptr<='F') || 
				(base==16 && *nptr>='a' && *nptr<='f') || 
				(base>=10 && *nptr>='0' && *nptr<='9') ||
				(base>=8 && *nptr>='0' && *nptr<='7') )
		{
			ret *= base;
			if(base==16 && *nptr>='A' && *nptr<='F')
				ret += *nptr-'A'+10;
			else if(base==16 && *nptr>='a' && *nptr<='f')
				ret += *nptr-'a'+10;
			else if(base>=10 && *nptr>='0' && *nptr<='9')
				ret += *nptr-'0';
			else if(base>=8 && *nptr>='0' && *nptr<='7')
				ret += *nptr-'0';
		}
		else
			return ret;
	}
	return ret;
}

static u8 GC2145MIPI_Initialize_from_T_Flash()
{
	//FS_HANDLE fp = -1;				/* Default, no file opened. */
	//u8 *data_buff = NULL;
	u8 *curr_ptr = NULL;
	u32 file_size = 0;
	//u32 bytes_read = 0;
	u32 i = 0, j = 0;
	u8 func_ind[4] = {0};	/* REG or DLY */

	printk("gc2145mipi___%s____11111111111111\n",__func__); 



	struct file *fp; 
	mm_segment_t fs; 
	loff_t pos = 0; 
	static u8 data_buff[10*1024] ;

	fp = filp_open("/mnt/sdcard/gc2145mipi_sd.txt", O_RDONLY , 0); 
	if (IS_ERR(fp)) 
	{ 
		printk("2145 create file error 1111111\n");  
		return -1; 
	} 
	else
	{
		printk("2145 create file error 2222222\n");  
	}
	fs = get_fs(); 
	set_fs(KERNEL_DS); 

	file_size = vfs_llseek(fp, 0, SEEK_END);
	vfs_read(fp, data_buff, file_size, &pos); 
	//printk("%s %d %d\n", buf,iFileLen,pos); 
	filp_close(fp, NULL); 
	set_fs(fs);


	printk("gc2145mipi___%s____22222222222222222\n",__func__); 



	/* Start parse the setting witch read from t-flash. */
	curr_ptr = data_buff;
	while (curr_ptr < (data_buff + file_size))
	{
		while ((*curr_ptr == ' ') || (*curr_ptr == '\t'))/* Skip the Space & TAB */
			curr_ptr++;				

		if (((*curr_ptr) == '/') && ((*(curr_ptr + 1)) == '*'))
		{
			while (!(((*curr_ptr) == '*') && ((*(curr_ptr + 1)) == '/')))
			{
				curr_ptr++;		/* Skip block comment code. */
			}

			while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
			{
				curr_ptr++;
			}

			curr_ptr += 2;						/* Skip the enter line */

			continue ;
		}

		if (((*curr_ptr) == '/') || ((*curr_ptr) == '{') || ((*curr_ptr) == '}'))		/* Comment line, skip it. */
		{
			while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
			{
				curr_ptr++;
			}

			curr_ptr += 2;						/* Skip the enter line */

			continue ;
		}
		/* This just content one enter line. */
		if (((*curr_ptr) == 0x0D) && ((*(curr_ptr + 1)) == 0x0A))
		{
			curr_ptr += 2;
			continue ;
		}
		//printk(" curr_ptr1 = %s\n",curr_ptr);
		memcpy(func_ind, curr_ptr, 3);


		if (strcmp((const char *)func_ind, "REG") == 0)		/* REG */
		{
			curr_ptr += 6;				/* Skip "REG(0x" or "DLY(" */
			gc2145mipi_Init_Reg[i].op_code = gc2145mipi_OP_CODE_REG;

			gc2145mipi_Init_Reg[i].init_reg = strtol((const char *)curr_ptr, 16);
			curr_ptr += 5;	/* Skip "00, 0x" */

			gc2145mipi_Init_Reg[i].init_val = strtol((const char *)curr_ptr, 16);
			curr_ptr += 4;	/* Skip "00);" */

		}
		else									/* DLY */
		{
			/* Need add delay for this setting. */ 
			curr_ptr += 4;	
			gc2145mipi_Init_Reg[i].op_code = gc2145mipi_OP_CODE_DLY;

			gc2145mipi_Init_Reg[i].init_reg = 0xFF;
			gc2145mipi_Init_Reg[i].init_val = strtol((const char *)curr_ptr,  10);	/* Get the delay ticks, the delay should less then 50 */
		}
		i++;


		/* Skip to next line directly. */
		while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
		{
			curr_ptr++;
		}
		curr_ptr += 2;
	}

	/* (0xFFFF, 0xFFFF) means the end of initial setting. */
	gc2145mipi_Init_Reg[i].op_code = gc2145mipi_OP_CODE_END;
	gc2145mipi_Init_Reg[i].init_reg = 0xFF;
	gc2145mipi_Init_Reg[i].init_val = 0xFF;
	i++;
	//for (j=0; j<i; j++)
	printk("gc2145mipi %x  ==  %x\n",gc2145mipi_Init_Reg[j].init_reg, gc2145mipi_Init_Reg[j].init_val);
	
	printk("gc2145mipi___%s____3333333333333333\n",__func__); 

	/* Start apply the initial setting to sensor. */
#if 1
	for (j=0; j<i; j++)
	{
		if (gc2145mipi_Init_Reg[j].op_code == gc2145mipi_OP_CODE_END)	/* End of the setting. */
		{
			printk("gc2145mipi REG OK -----------------END!\n");
		
			break ;
		}
		else if (gc2145mipi_Init_Reg[j].op_code == gc2145mipi_OP_CODE_DLY)
		{
			msleep(gc2145mipi_Init_Reg[j].init_val);		/* Delay */
			printk("gc2145mipi REG OK -----------------DLY!\n");			
		}
		else if (gc2145mipi_Init_Reg[j].op_code == gc2145mipi_OP_CODE_REG)
		{

			GC2145MIPI_write_cmos_sensor(gc2145mipi_Init_Reg[j].init_reg, gc2145mipi_Init_Reg[j].init_val);
			printk("gc2145mipi REG OK!-----------------REG(0x%x,0x%x)\n",gc2145mipi_Init_Reg[j].init_reg, gc2145mipi_Init_Reg[j].init_val);			
			printk("gc2145mipi REG OK!-----------------REG(0x%x,0x%x)\n",gc2145mipi_Init_Reg[j].init_reg, gc2145mipi_Init_Reg[j].init_val);			
			printk("gc2145mipi REG OK!-----------------REG(0x%x,0x%x)\n",gc2145mipi_Init_Reg[j].init_reg, gc2145mipi_Init_Reg[j].init_val);			
			
		}
		else
		{
			printk("gc2145mipi REG ERROR!\n");
		}
	}
#endif
	return 1;	
}

#endif

/*******************************************************************************
* // Adapter for Winmo typedef 
********************************************************************************/
#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT


/*******************************************************************************
* // End Adapter for Winmo typedef 
********************************************************************************/
/* Global Valuable */

static kal_uint32 zoom_factor = 0; 

static kal_bool GC2145MIPI_VEDIO_encode_mode = KAL_FALSE; //Picture(Jpeg) or Video(Mpeg4)
static kal_bool GC2145MIPI_sensor_cap_state = KAL_FALSE; //Preview or Capture

static kal_uint16 GC2145MIPI_exposure_lines=0, GC2145MIPI_extra_exposure_lines = 0;

static kal_uint16 GC2145MIPI_Capture_Shutter=0;
static kal_uint16 GC2145MIPI_Capture_Extra_Lines=0;

kal_uint32 GC2145MIPI_capture_pclk_in_M=520,GC2145MIPI_preview_pclk_in_M=390,GC2145MIPI_PV_dummy_pixels=0,GC2145MIPI_PV_dummy_lines=0,GC2145MIPI_isp_master_clock=0;

static kal_uint32  GC2145MIPI_sensor_pclk=390;

static kal_uint32 Preview_Shutter = 0;
static kal_uint32 Capture_Shutter = 0;

MSDK_SENSOR_CONFIG_STRUCT GC2145MIPISensorConfigData;

kal_uint16 GC2145MIPI_read_shutter(void)
{
	return  (GC2145MIPI_read_cmos_sensor(0x03) << 8)|GC2145MIPI_read_cmos_sensor(0x04) ;
} /* GC2145MIPI read_shutter */



static void GC2145MIPI_write_shutter(kal_uint32 shutter)
{

	if(shutter < 1)	
 	return;

	GC2145MIPI_write_cmos_sensor(0x03, (shutter >> 8) & 0x1f);
	GC2145MIPI_write_cmos_sensor(0x04, shutter & 0xff);
}    /* GC2145MIPI_write_shutter */


static void GC2145MIPI_set_mirror_flip(kal_uint8 image_mirror)
{
	kal_uint8 GC2145MIPI_HV_Mirror;

	switch (image_mirror) 
	{
		case IMAGE_NORMAL:
			GC2145MIPI_HV_Mirror = 0x14; 
		    break;
		case IMAGE_H_MIRROR:
			GC2145MIPI_HV_Mirror = 0x15;
		    break;
		case IMAGE_V_MIRROR:
			GC2145MIPI_HV_Mirror = 0x16; 
		    break;
		case IMAGE_HV_MIRROR:
			GC2145MIPI_HV_Mirror = 0x17;
		    break;
		default:
		    break;
	}
	GC2145MIPI_write_cmos_sensor(0x17, GC2145MIPI_HV_Mirror);
}

static void GC2145MIPI_set_AE_mode(kal_bool AE_enable)
{
	kal_uint8 temp_AE_reg = 0;

	GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
	if (AE_enable == KAL_TRUE)
	{
		// turn on AEC/AGC
		GC2145MIPI_write_cmos_sensor(0xb6, 0x01);
	}
	else
	{
		// turn off AEC/AGC
		GC2145MIPI_write_cmos_sensor(0xb6, 0x00);
	}
}


static void GC2145MIPI_set_AWB_mode(kal_bool AWB_enable)
{
	kal_uint8 temp_AWB_reg = 0;

	GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
	temp_AWB_reg = GC2145MIPI_read_cmos_sensor(0x82);
	if (AWB_enable == KAL_TRUE)
	{
		//enable Auto WB
		temp_AWB_reg = temp_AWB_reg | 0x02;
	}
	else
	{
		//turn off AWB
		temp_AWB_reg = temp_AWB_reg & 0xfd;
	}
	GC2145MIPI_write_cmos_sensor(0x82, temp_AWB_reg);
}


/*************************************************************************
* FUNCTION
*	GC2145MIPI_night_mode
*
* DESCRIPTION
*	This function night mode of GC2145MIPI.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void GC2145MIPI_night_mode(kal_bool enable)
{
	
		/* ==Video Preview, Auto Mode, use 39MHz PCLK, 30fps; Night Mode use 39M, 15fps */
		if (GC2145MIPI_sensor_cap_state == KAL_FALSE) 
		{
			if (enable) 
			{
				if (GC2145MIPI_VEDIO_encode_mode == KAL_TRUE) 
				{
					GC2145MIPI_write_cmos_sensor(0xfe, 0x01);
					GC2145MIPI_write_cmos_sensor(0x3c, 0x60);
					GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
				}
				else 
				{
					GC2145MIPI_write_cmos_sensor(0xfe, 0x01);
					GC2145MIPI_write_cmos_sensor(0x3c, 0x60);
					GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
				}
			}
			else 
			{
				/* when enter normal mode (disable night mode) without light, the AE vibrate */
				if (GC2145MIPI_VEDIO_encode_mode == KAL_TRUE) 
				{
					GC2145MIPI_write_cmos_sensor(0xfe, 0x01);
					GC2145MIPI_write_cmos_sensor(0x3c, 0x40);
					GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
				}
				else 
				{
					GC2145MIPI_write_cmos_sensor(0xfe, 0x01);
					GC2145MIPI_write_cmos_sensor(0x3c, 0x40);
					GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
				}
		}
	}
}	/* GC2145MIPI_night_mode */



/*************************************************************************
* FUNCTION
*	GC2145MIPI_GetSensorID
*
* DESCRIPTION
*	This function get the sensor ID
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 GC2145MIPI_GetSensorID(kal_uint32 *sensorID)

{
	   int  retry = 3; 
    // check if sensor ID correct
    do {
	
	*sensorID=((GC2145MIPI_read_cmos_sensor(0xf0) << 8) | GC2145MIPI_read_cmos_sensor(0xf1));	
	 if (*sensorID == GC2145_SENSOR_ID)
            break; 

	SENSORDB("GC2145MIPI_GetSensorID:%x \n",*sensorID);
	retry--;

	  } while (retry > 0);
	
	if (*sensorID != GC2145_SENSOR_ID) {		
		*sensorID = 0xFFFFFFFF;		
		return ERROR_SENSOR_CONNECT_FAIL;
	}
   return ERROR_NONE;
}   /* GC2145MIPIOpen  */

static void GC2145MIPI_Sensor_Init(void)
{
	zoom_factor = 0; 
	SENSORDB("GC2145MIPI_Sensor_Init");
	GC2145MIPI_write_cmos_sensor(0xfe, 0xf0);
	GC2145MIPI_write_cmos_sensor(0xfe, 0xf0);
	GC2145MIPI_write_cmos_sensor(0xfe, 0xf0);
	GC2145MIPI_write_cmos_sensor(0xfc, 0x06);
	GC2145MIPI_write_cmos_sensor(0xf6, 0x00);
	GC2145MIPI_write_cmos_sensor(0xf7, 0x1d);
	GC2145MIPI_write_cmos_sensor(0xf8, 0x84);
	GC2145MIPI_write_cmos_sensor(0xfa, 0x00);
	GC2145MIPI_write_cmos_sensor(0xf9, 0x8e);
	GC2145MIPI_write_cmos_sensor(0xf2, 0x00);
	/////////////////////////////////////////////////
	//////////////////ISP reg//////////////////////
	////////////////////////////////////////////////////
	GC2145MIPI_write_cmos_sensor(0xfe , 0x00);
	GC2145MIPI_write_cmos_sensor(0x03 , 0x04);
	GC2145MIPI_write_cmos_sensor(0x04 , 0xe2);
	GC2145MIPI_write_cmos_sensor(0x09 , 0x00);
	GC2145MIPI_write_cmos_sensor(0x0a , 0x00);
	GC2145MIPI_write_cmos_sensor(0x0b , 0x00);
	GC2145MIPI_write_cmos_sensor(0x0c , 0x00);
	GC2145MIPI_write_cmos_sensor(0x0d , 0x04);
	GC2145MIPI_write_cmos_sensor(0x0e , 0xc0);
	GC2145MIPI_write_cmos_sensor(0x0f , 0x06);
	GC2145MIPI_write_cmos_sensor(0x10 , 0x52);
	GC2145MIPI_write_cmos_sensor(0x12 , 0x2e);
	GC2145MIPI_write_cmos_sensor(0x17 , 0x14); //mirror
	GC2145MIPI_write_cmos_sensor(0x18 , 0x22);
	GC2145MIPI_write_cmos_sensor(0x19 , 0x0e);
	GC2145MIPI_write_cmos_sensor(0x1a , 0x01);
	GC2145MIPI_write_cmos_sensor(0x1b , 0x4b);
	GC2145MIPI_write_cmos_sensor(0x1c , 0x07);
	GC2145MIPI_write_cmos_sensor(0x1d , 0x10);
	GC2145MIPI_write_cmos_sensor(0x1e , 0x88);
	GC2145MIPI_write_cmos_sensor(0x1f , 0x78);
	GC2145MIPI_write_cmos_sensor(0x20 , 0x03);
	GC2145MIPI_write_cmos_sensor(0x21 , 0x40);
	GC2145MIPI_write_cmos_sensor(0x22 , 0xa0); 
	GC2145MIPI_write_cmos_sensor(0x24 , 0x16);
	GC2145MIPI_write_cmos_sensor(0x25 , 0x01);
	GC2145MIPI_write_cmos_sensor(0x26 , 0x10);
	GC2145MIPI_write_cmos_sensor(0x2d , 0x60);
	GC2145MIPI_write_cmos_sensor(0x30 , 0x01);
	GC2145MIPI_write_cmos_sensor(0x31 , 0x90);
	GC2145MIPI_write_cmos_sensor(0x33 , 0x06);
	GC2145MIPI_write_cmos_sensor(0x34 , 0x01);
	/////////////////////////////////////////////////
	//////////////////ISP reg////////////////////
	/////////////////////////////////////////////////
	GC2145MIPI_write_cmos_sensor(0xfe , 0x00);
	GC2145MIPI_write_cmos_sensor(0x80 , 0x7f);
	GC2145MIPI_write_cmos_sensor(0x81 , 0x26);
	GC2145MIPI_write_cmos_sensor(0x82 , 0xfa);
	GC2145MIPI_write_cmos_sensor(0x83 , 0x00);
	GC2145MIPI_write_cmos_sensor(0x84 , 0x03); 
	GC2145MIPI_write_cmos_sensor(0x86 , 0x02);
	GC2145MIPI_write_cmos_sensor(0x88 , 0x03);
	GC2145MIPI_write_cmos_sensor(0x89 , 0x03);
	GC2145MIPI_write_cmos_sensor(0x85 , 0x08); 
	GC2145MIPI_write_cmos_sensor(0x8a , 0x00);
	GC2145MIPI_write_cmos_sensor(0x8b , 0x00);
	GC2145MIPI_write_cmos_sensor(0xb0 , 0x55);
	GC2145MIPI_write_cmos_sensor(0xc3 , 0x00);
	GC2145MIPI_write_cmos_sensor(0xc4 , 0x80);
	GC2145MIPI_write_cmos_sensor(0xc5 , 0x90);
	GC2145MIPI_write_cmos_sensor(0xc6 , 0x3b);
	GC2145MIPI_write_cmos_sensor(0xc7 , 0x46);
	GC2145MIPI_write_cmos_sensor(0xec , 0x06);
	GC2145MIPI_write_cmos_sensor(0xed , 0x04);
	GC2145MIPI_write_cmos_sensor(0xee , 0x60);
	GC2145MIPI_write_cmos_sensor(0xef , 0x90);
	GC2145MIPI_write_cmos_sensor(0xb6 , 0x01);
	GC2145MIPI_write_cmos_sensor(0x90 , 0x01);
	GC2145MIPI_write_cmos_sensor(0x91 , 0x00);
	GC2145MIPI_write_cmos_sensor(0x92 , 0x00);
	GC2145MIPI_write_cmos_sensor(0x93 , 0x00);
	GC2145MIPI_write_cmos_sensor(0x94 , 0x00);
	GC2145MIPI_write_cmos_sensor(0x95 , 0x04);
	GC2145MIPI_write_cmos_sensor(0x96 , 0xb0);
	GC2145MIPI_write_cmos_sensor(0x97 , 0x06);
	GC2145MIPI_write_cmos_sensor(0x98 , 0x40);
	/////////////////////////////////////////
	/////////// BLK ////////////////////////
	/////////////////////////////////////////
	GC2145MIPI_write_cmos_sensor(0xfe , 0x00);
	GC2145MIPI_write_cmos_sensor(0x40 , 0x42);
	GC2145MIPI_write_cmos_sensor(0x41 , 0x00);
	GC2145MIPI_write_cmos_sensor(0x43 , 0x5b); 
	GC2145MIPI_write_cmos_sensor(0x5e , 0x00); 
	GC2145MIPI_write_cmos_sensor(0x5f , 0x00);
	GC2145MIPI_write_cmos_sensor(0x60 , 0x00); 
	GC2145MIPI_write_cmos_sensor(0x61 , 0x00); 
	GC2145MIPI_write_cmos_sensor(0x62 , 0x00);
	GC2145MIPI_write_cmos_sensor(0x63 , 0x00); 
	GC2145MIPI_write_cmos_sensor(0x64 , 0x00); 
	GC2145MIPI_write_cmos_sensor(0x65 , 0x00); 
	GC2145MIPI_write_cmos_sensor(0x66 , 0x20);
	GC2145MIPI_write_cmos_sensor(0x67 , 0x20); 
	GC2145MIPI_write_cmos_sensor(0x68 , 0x20); 
	GC2145MIPI_write_cmos_sensor(0x69 , 0x20); 
	GC2145MIPI_write_cmos_sensor(0x76 , 0x00);                                  
	GC2145MIPI_write_cmos_sensor(0x6a , 0x08); 
	GC2145MIPI_write_cmos_sensor(0x6b , 0x08); 
	GC2145MIPI_write_cmos_sensor(0x6c , 0x08); 
	GC2145MIPI_write_cmos_sensor(0x6d , 0x08); 
	GC2145MIPI_write_cmos_sensor(0x6e , 0x08); 
	GC2145MIPI_write_cmos_sensor(0x6f , 0x08); 
	GC2145MIPI_write_cmos_sensor(0x70 , 0x08); 
	GC2145MIPI_write_cmos_sensor(0x71 , 0x08);   
	GC2145MIPI_write_cmos_sensor(0x76 , 0x00);
	GC2145MIPI_write_cmos_sensor(0x72 , 0xf0);
	GC2145MIPI_write_cmos_sensor(0x7e , 0x3c);
	GC2145MIPI_write_cmos_sensor(0x7f , 0x00);
	GC2145MIPI_write_cmos_sensor(0xfe , 0x02);
	GC2145MIPI_write_cmos_sensor(0x48 , 0x15);
	GC2145MIPI_write_cmos_sensor(0x49 , 0x00);
	GC2145MIPI_write_cmos_sensor(0x4b , 0x0b);
	GC2145MIPI_write_cmos_sensor(0xfe , 0x00);
	////////////////////////////////////////
	/////////// AEC ////////////////////////
	////////////////////////////////////////
	GC2145MIPI_write_cmos_sensor(0xfe , 0x01);
	GC2145MIPI_write_cmos_sensor(0x01 , 0x04);
	GC2145MIPI_write_cmos_sensor(0x02 , 0xc0);
	GC2145MIPI_write_cmos_sensor(0x03 , 0x04);
	GC2145MIPI_write_cmos_sensor(0x04 , 0x90);
	GC2145MIPI_write_cmos_sensor(0x05 , 0x30);
	GC2145MIPI_write_cmos_sensor(0x06 , 0x90);
	GC2145MIPI_write_cmos_sensor(0x07 , 0x30);
	GC2145MIPI_write_cmos_sensor(0x08 , 0x80);
	GC2145MIPI_write_cmos_sensor(0x09 , 0x00);
	GC2145MIPI_write_cmos_sensor(0x0a , 0x82);
	GC2145MIPI_write_cmos_sensor(0x0b , 0x11);
	GC2145MIPI_write_cmos_sensor(0x0c , 0x10);
	GC2145MIPI_write_cmos_sensor(0x11 , 0x10);
	GC2145MIPI_write_cmos_sensor(0x13 , 0x7b);
	GC2145MIPI_write_cmos_sensor(0x17 , 0x00);
	GC2145MIPI_write_cmos_sensor(0x1c , 0x11);
	GC2145MIPI_write_cmos_sensor(0x1e , 0x61);
	GC2145MIPI_write_cmos_sensor(0x1f , 0x35);
	GC2145MIPI_write_cmos_sensor(0x20 , 0x40);
	GC2145MIPI_write_cmos_sensor(0x22 , 0x40);
	GC2145MIPI_write_cmos_sensor(0x23 , 0x20);
	GC2145MIPI_write_cmos_sensor(0xfe , 0x02);
	GC2145MIPI_write_cmos_sensor(0x0f , 0x04);
	GC2145MIPI_write_cmos_sensor(0xfe , 0x01);
	GC2145MIPI_write_cmos_sensor(0x12 , 0x35);
	GC2145MIPI_write_cmos_sensor(0x15 , 0xb0);
	GC2145MIPI_write_cmos_sensor(0x10 , 0x31);
	GC2145MIPI_write_cmos_sensor(0x3e , 0x28);
	GC2145MIPI_write_cmos_sensor(0x3f , 0xb0);
	GC2145MIPI_write_cmos_sensor(0x40 , 0x90);
	GC2145MIPI_write_cmos_sensor(0x41 , 0x0f);
	
	/////////////////////////////
	//////// INTPEE /////////////
	/////////////////////////////
	GC2145MIPI_write_cmos_sensor(0xfe , 0x02);
	GC2145MIPI_write_cmos_sensor(0x90 , 0x6c);
	GC2145MIPI_write_cmos_sensor(0x91 , 0x03);
	GC2145MIPI_write_cmos_sensor(0x92 , 0xcb);
	GC2145MIPI_write_cmos_sensor(0x94 , 0x33);
	GC2145MIPI_write_cmos_sensor(0x95 , 0x84);
	GC2145MIPI_write_cmos_sensor(0x97 , 0x65);
	GC2145MIPI_write_cmos_sensor(0xa2 , 0x11);
	GC2145MIPI_write_cmos_sensor(0xfe , 0x00);
	/////////////////////////////
	//////// DNDD///////////////
	/////////////////////////////
	GC2145MIPI_write_cmos_sensor(0xfe , 0x02);
	GC2145MIPI_write_cmos_sensor(0x80 , 0xc1);
	GC2145MIPI_write_cmos_sensor(0x81 , 0x08);
	GC2145MIPI_write_cmos_sensor(0x82 , 0x05);
	GC2145MIPI_write_cmos_sensor(0x83 , 0x08);
	GC2145MIPI_write_cmos_sensor(0x84 , 0x0a);
	GC2145MIPI_write_cmos_sensor(0x86 , 0xf0);
	GC2145MIPI_write_cmos_sensor(0x87 , 0x50);
	GC2145MIPI_write_cmos_sensor(0x88 , 0x15);
	GC2145MIPI_write_cmos_sensor(0x89 , 0xb0);
	GC2145MIPI_write_cmos_sensor(0x8a , 0x30);
	GC2145MIPI_write_cmos_sensor(0x8b , 0x10);
	/////////////////////////////////////////
	/////////// ASDE ////////////////////////
	/////////////////////////////////////////
	GC2145MIPI_write_cmos_sensor(0xfe , 0x01);
	GC2145MIPI_write_cmos_sensor(0x21 , 0x04);
	GC2145MIPI_write_cmos_sensor(0xfe , 0x02);
	GC2145MIPI_write_cmos_sensor(0xa3 , 0x50);
	GC2145MIPI_write_cmos_sensor(0xa4 , 0x20);
	GC2145MIPI_write_cmos_sensor(0xa5 , 0x40);
	GC2145MIPI_write_cmos_sensor(0xa6 , 0x80);
	GC2145MIPI_write_cmos_sensor(0xab , 0x40);
	GC2145MIPI_write_cmos_sensor(0xae , 0x0c);
	GC2145MIPI_write_cmos_sensor(0xb3 , 0x46);
	GC2145MIPI_write_cmos_sensor(0xb4 , 0x64);
	GC2145MIPI_write_cmos_sensor(0xb6 , 0x38);
	GC2145MIPI_write_cmos_sensor(0xb7 , 0x01);
	GC2145MIPI_write_cmos_sensor(0xb9 , 0x2b);
	GC2145MIPI_write_cmos_sensor(0x3c , 0x04);
	GC2145MIPI_write_cmos_sensor(0x3d , 0x15);
	GC2145MIPI_write_cmos_sensor(0x4b , 0x06);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x20);
	GC2145MIPI_write_cmos_sensor(0xfe , 0x00);
	/////////////////////////////////////////
	/////////// GAMMA   ////////////////////////
	/////////////////////////////////////////
	
	///////////////////gamma1////////////////////
	#if 1
	GC2145MIPI_write_cmos_sensor(0xfe , 0x02);
	GC2145MIPI_write_cmos_sensor(0x10 , 0x09);
	GC2145MIPI_write_cmos_sensor(0x11 , 0x0d);
	GC2145MIPI_write_cmos_sensor(0x12 , 0x13);
	GC2145MIPI_write_cmos_sensor(0x13 , 0x19);
	GC2145MIPI_write_cmos_sensor(0x14 , 0x27);
	GC2145MIPI_write_cmos_sensor(0x15 , 0x37);
	GC2145MIPI_write_cmos_sensor(0x16 , 0x45);
	GC2145MIPI_write_cmos_sensor(0x17 , 0x53);
	GC2145MIPI_write_cmos_sensor(0x18 , 0x69);
	GC2145MIPI_write_cmos_sensor(0x19 , 0x7d);
	GC2145MIPI_write_cmos_sensor(0x1a , 0x8f);
	GC2145MIPI_write_cmos_sensor(0x1b , 0x9d);
	GC2145MIPI_write_cmos_sensor(0x1c , 0xa9);
	GC2145MIPI_write_cmos_sensor(0x1d , 0xbd);
	GC2145MIPI_write_cmos_sensor(0x1e , 0xcd);
	GC2145MIPI_write_cmos_sensor(0x1f , 0xd9);
	GC2145MIPI_write_cmos_sensor(0x20 , 0xe3);
	GC2145MIPI_write_cmos_sensor(0x21 , 0xea);
	GC2145MIPI_write_cmos_sensor(0x22 , 0xef);
	GC2145MIPI_write_cmos_sensor(0x23 , 0xf5);
	GC2145MIPI_write_cmos_sensor(0x24 , 0xf9);
	GC2145MIPI_write_cmos_sensor(0x25 , 0xff);
	#else                               
	GC2145MIPI_write_cmos_sensor(0xfe , 0x02);
	GC2145MIPI_write_cmos_sensor(0x10 , 0x0a);
	GC2145MIPI_write_cmos_sensor(0x11 , 0x12);
	GC2145MIPI_write_cmos_sensor(0x12 , 0x19);
	GC2145MIPI_write_cmos_sensor(0x13 , 0x1f);
	GC2145MIPI_write_cmos_sensor(0x14 , 0x2c);
	GC2145MIPI_write_cmos_sensor(0x15 , 0x38);
	GC2145MIPI_write_cmos_sensor(0x16 , 0x42);
	GC2145MIPI_write_cmos_sensor(0x17 , 0x4e);
	GC2145MIPI_write_cmos_sensor(0x18 , 0x63);
	GC2145MIPI_write_cmos_sensor(0x19 , 0x76);
	GC2145MIPI_write_cmos_sensor(0x1a , 0x87);
	GC2145MIPI_write_cmos_sensor(0x1b , 0x96);
	GC2145MIPI_write_cmos_sensor(0x1c , 0xa2);
	GC2145MIPI_write_cmos_sensor(0x1d , 0xb8);
	GC2145MIPI_write_cmos_sensor(0x1e , 0xcb);
	GC2145MIPI_write_cmos_sensor(0x1f , 0xd8);
	GC2145MIPI_write_cmos_sensor(0x20 , 0xe2);
	GC2145MIPI_write_cmos_sensor(0x21 , 0xe9);
	GC2145MIPI_write_cmos_sensor(0x22 , 0xf0);
	GC2145MIPI_write_cmos_sensor(0x23 , 0xf8);
	GC2145MIPI_write_cmos_sensor(0x24 , 0xfd);
	GC2145MIPI_write_cmos_sensor(0x25 , 0xff);
	GC2145MIPI_write_cmos_sensor(0xfe , 0x00);     
	#endif 
	GC2145MIPI_write_cmos_sensor(0xfe , 0x00);     
	GC2145MIPI_write_cmos_sensor(0xc6 , 0x20);
	GC2145MIPI_write_cmos_sensor(0xc7 , 0x2b);
	///////////////////gamma2////////////////////
	#if 1
	GC2145MIPI_write_cmos_sensor(0xfe , 0x02);
	GC2145MIPI_write_cmos_sensor(0x26 , 0x0f);
	GC2145MIPI_write_cmos_sensor(0x27 , 0x14);
	GC2145MIPI_write_cmos_sensor(0x28 , 0x19);
	GC2145MIPI_write_cmos_sensor(0x29 , 0x1e);
	GC2145MIPI_write_cmos_sensor(0x2a , 0x27);
	GC2145MIPI_write_cmos_sensor(0x2b , 0x33);
	GC2145MIPI_write_cmos_sensor(0x2c , 0x3b);
	GC2145MIPI_write_cmos_sensor(0x2d , 0x45);
	GC2145MIPI_write_cmos_sensor(0x2e , 0x59);
	GC2145MIPI_write_cmos_sensor(0x2f , 0x69);
	GC2145MIPI_write_cmos_sensor(0x30 , 0x7c);
	GC2145MIPI_write_cmos_sensor(0x31 , 0x89);
	GC2145MIPI_write_cmos_sensor(0x32 , 0x98);
	GC2145MIPI_write_cmos_sensor(0x33 , 0xae);
	GC2145MIPI_write_cmos_sensor(0x34 , 0xc0);
	GC2145MIPI_write_cmos_sensor(0x35 , 0xcf);
	GC2145MIPI_write_cmos_sensor(0x36 , 0xda);
	GC2145MIPI_write_cmos_sensor(0x37 , 0xe2);
	GC2145MIPI_write_cmos_sensor(0x38 , 0xe9);
	GC2145MIPI_write_cmos_sensor(0x39 , 0xf3);
	GC2145MIPI_write_cmos_sensor(0x3a , 0xf9);
	GC2145MIPI_write_cmos_sensor(0x3b , 0xff);
	#else
	////Gamma outdoor
	GC2145MIPI_write_cmos_sensor(0xfe , 0x02);
	GC2145MIPI_write_cmos_sensor(0x26 , 0x17);
	GC2145MIPI_write_cmos_sensor(0x27 , 0x18);
	GC2145MIPI_write_cmos_sensor(0x28 , 0x1c);
	GC2145MIPI_write_cmos_sensor(0x29 , 0x20);
	GC2145MIPI_write_cmos_sensor(0x2a , 0x28);
	GC2145MIPI_write_cmos_sensor(0x2b , 0x34);
	GC2145MIPI_write_cmos_sensor(0x2c , 0x40);
	GC2145MIPI_write_cmos_sensor(0x2d , 0x49);
	GC2145MIPI_write_cmos_sensor(0x2e , 0x5b);
	GC2145MIPI_write_cmos_sensor(0x2f , 0x6d);
	GC2145MIPI_write_cmos_sensor(0x30 , 0x7d);
	GC2145MIPI_write_cmos_sensor(0x31 , 0x89);
	GC2145MIPI_write_cmos_sensor(0x32 , 0x97);
	GC2145MIPI_write_cmos_sensor(0x33 , 0xac);
	GC2145MIPI_write_cmos_sensor(0x34 , 0xc0);
	GC2145MIPI_write_cmos_sensor(0x35 , 0xcf);
	GC2145MIPI_write_cmos_sensor(0x36 , 0xda);
	GC2145MIPI_write_cmos_sensor(0x37 , 0xe5);
	GC2145MIPI_write_cmos_sensor(0x38 , 0xec);
	GC2145MIPI_write_cmos_sensor(0x39 , 0xf8);
	GC2145MIPI_write_cmos_sensor(0x3a , 0xfd);
	GC2145MIPI_write_cmos_sensor(0x3b , 0xff);
	#endif
	/////////////////////////////////////////////// 
	///////////YCP /////////////////////// 
	/////////////////////////////////////////////// 
	GC2145MIPI_write_cmos_sensor(0xfe , 0x02);
	GC2145MIPI_write_cmos_sensor(0xd1 , 0x32);
	GC2145MIPI_write_cmos_sensor(0xd2 , 0x32);
	GC2145MIPI_write_cmos_sensor(0xd3 , 0x40);
	GC2145MIPI_write_cmos_sensor(0xd6 , 0xf0);
	GC2145MIPI_write_cmos_sensor(0xd7 , 0x10);
	GC2145MIPI_write_cmos_sensor(0xd8 , 0xda);
	GC2145MIPI_write_cmos_sensor(0xdd , 0x14);
	GC2145MIPI_write_cmos_sensor(0xde , 0x86);
	GC2145MIPI_write_cmos_sensor(0xed , 0x80);
	GC2145MIPI_write_cmos_sensor(0xee , 0x00);
	GC2145MIPI_write_cmos_sensor(0xef , 0x3f);
	GC2145MIPI_write_cmos_sensor(0xd8 , 0xd8);
	///////////////////abs/////////////////
	GC2145MIPI_write_cmos_sensor(0xfe , 0x01);
	GC2145MIPI_write_cmos_sensor(0x9f , 0x40);
	/////////////////////////////////////////////
	//////////////////////// LSC ///////////////
	//////////////////////////////////////////
	GC2145MIPI_write_cmos_sensor(0xfe , 0x01);
	GC2145MIPI_write_cmos_sensor(0xc2 , 0x14);
	GC2145MIPI_write_cmos_sensor(0xc3 , 0x0d);
	GC2145MIPI_write_cmos_sensor(0xc4 , 0x0c);
	GC2145MIPI_write_cmos_sensor(0xc8 , 0x15);
	GC2145MIPI_write_cmos_sensor(0xc9 , 0x0d);
	GC2145MIPI_write_cmos_sensor(0xca , 0x0a);
	GC2145MIPI_write_cmos_sensor(0xbc , 0x24);
	GC2145MIPI_write_cmos_sensor(0xbd , 0x10);
	GC2145MIPI_write_cmos_sensor(0xbe , 0x0b);
	GC2145MIPI_write_cmos_sensor(0xb6 , 0x25);
	GC2145MIPI_write_cmos_sensor(0xb7 , 0x16);
	GC2145MIPI_write_cmos_sensor(0xb8 , 0x15);
	GC2145MIPI_write_cmos_sensor(0xc5 , 0x00);
	GC2145MIPI_write_cmos_sensor(0xc6 , 0x00);
	GC2145MIPI_write_cmos_sensor(0xc7 , 0x00);
	GC2145MIPI_write_cmos_sensor(0xcb , 0x00);
	GC2145MIPI_write_cmos_sensor(0xcc , 0x00);
	GC2145MIPI_write_cmos_sensor(0xcd , 0x00);
	GC2145MIPI_write_cmos_sensor(0xbf , 0x07);
	GC2145MIPI_write_cmos_sensor(0xc0 , 0x00);
	GC2145MIPI_write_cmos_sensor(0xc1 , 0x00);
	GC2145MIPI_write_cmos_sensor(0xb9 , 0x00);
	GC2145MIPI_write_cmos_sensor(0xba , 0x00);
	GC2145MIPI_write_cmos_sensor(0xbb , 0x00);
	GC2145MIPI_write_cmos_sensor(0xaa , 0x01);
	GC2145MIPI_write_cmos_sensor(0xab , 0x01);
	GC2145MIPI_write_cmos_sensor(0xac , 0x00);
	GC2145MIPI_write_cmos_sensor(0xad , 0x05);
	GC2145MIPI_write_cmos_sensor(0xae , 0x06);
	GC2145MIPI_write_cmos_sensor(0xaf , 0x0e);
	GC2145MIPI_write_cmos_sensor(0xb0 , 0x0b);
	GC2145MIPI_write_cmos_sensor(0xb1 , 0x07);
	GC2145MIPI_write_cmos_sensor(0xb2 , 0x06);
	GC2145MIPI_write_cmos_sensor(0xb3 , 0x17);
	GC2145MIPI_write_cmos_sensor(0xb4 , 0x0e);
	GC2145MIPI_write_cmos_sensor(0xb5 , 0x0e);
	GC2145MIPI_write_cmos_sensor(0xd0 , 0x09);
	GC2145MIPI_write_cmos_sensor(0xd1 , 0x00);
	GC2145MIPI_write_cmos_sensor(0xd2 , 0x00);
	GC2145MIPI_write_cmos_sensor(0xd6 , 0x08);
	GC2145MIPI_write_cmos_sensor(0xd7 , 0x00);
	GC2145MIPI_write_cmos_sensor(0xd8 , 0x00);
	GC2145MIPI_write_cmos_sensor(0xd9 , 0x00);
	GC2145MIPI_write_cmos_sensor(0xda , 0x00);
	GC2145MIPI_write_cmos_sensor(0xdb , 0x00);
	GC2145MIPI_write_cmos_sensor(0xd3 , 0x0a);
	GC2145MIPI_write_cmos_sensor(0xd4 , 0x00);
	GC2145MIPI_write_cmos_sensor(0xd5 , 0x00);
	GC2145MIPI_write_cmos_sensor(0xa4 , 0x00);
	GC2145MIPI_write_cmos_sensor(0xa5 , 0x00);
	GC2145MIPI_write_cmos_sensor(0xa6 , 0x77);
	GC2145MIPI_write_cmos_sensor(0xa7 , 0x77);
	GC2145MIPI_write_cmos_sensor(0xa8 , 0x77);
	GC2145MIPI_write_cmos_sensor(0xa9 , 0x77);
	GC2145MIPI_write_cmos_sensor(0xa1 , 0x80);
	GC2145MIPI_write_cmos_sensor(0xa2 , 0x80);
	               
	GC2145MIPI_write_cmos_sensor(0xfe , 0x01);
	GC2145MIPI_write_cmos_sensor(0xdf , 0x0d);
	GC2145MIPI_write_cmos_sensor(0xdc , 0x25);
	GC2145MIPI_write_cmos_sensor(0xdd , 0x30);
	GC2145MIPI_write_cmos_sensor(0xe0 , 0x77);
	GC2145MIPI_write_cmos_sensor(0xe1 , 0x80);
	GC2145MIPI_write_cmos_sensor(0xe2 , 0x77);
	GC2145MIPI_write_cmos_sensor(0xe3 , 0x90);
	GC2145MIPI_write_cmos_sensor(0xe6 , 0x90);
	GC2145MIPI_write_cmos_sensor(0xe7 , 0xa0);
	GC2145MIPI_write_cmos_sensor(0xe8 , 0x90);
	GC2145MIPI_write_cmos_sensor(0xe9 , 0xa0);                                      
	GC2145MIPI_write_cmos_sensor(0xfe , 0x00);
	///////////////////////////////////////////////
	/////////// AWB////////////////////////
	///////////////////////////////////////////////
	GC2145MIPI_write_cmos_sensor(0xfe , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4f , 0x00);
	GC2145MIPI_write_cmos_sensor(0x4f , 0x00);
	GC2145MIPI_write_cmos_sensor(0x4b , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4f , 0x00);
	         
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01); // D75
	GC2145MIPI_write_cmos_sensor(0x4d , 0x71);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x91);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x70);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x01);
	         
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01); // D65
	GC2145MIPI_write_cmos_sensor(0x4d , 0x90);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x02);                                    
	         
	         
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xb0);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x8f);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x6f);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xaf);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x02);
	         
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xd0);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xf0);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xcf);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xef);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x02);
	         
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);//D50
	GC2145MIPI_write_cmos_sensor(0x4d , 0x6e);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x03);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01); 
	GC2145MIPI_write_cmos_sensor(0x4d , 0x8e);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x03);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xae);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x03);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xce);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x03);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x4d);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x03);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x6d);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x03);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x8d);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x03);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xad);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x03);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xcd);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x03);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x4c);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x03);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x6c);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x03);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x8c);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x03);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xac);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x03);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xcc);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x03);
	         
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xcb);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x03);
	         
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x4b);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x03);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x6b);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x03);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x8b);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x03);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xab);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x03);
	         
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);//CWF
	GC2145MIPI_write_cmos_sensor(0x4d , 0x8a);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x04);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xaa);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x04);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xca);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x04);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xca);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x04);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xc9);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x04);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x8a);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x04);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x89);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x04);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xa9);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x04);
	         
	         
	         
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);//tl84
	GC2145MIPI_write_cmos_sensor(0x4d , 0x0b);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x05);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x0a);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x05);
	         
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xeb);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x05);
	         
	GC2145MIPI_write_cmos_sensor(0x4c , 0x01);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xea);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x05);
	                 
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x09);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x05);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x29);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x05);
	                     
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x2a);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x05);
	                      
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x4a);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x05);
	
	//GC2145MIPI_write_cmos_sensor(0x4c , 0x02); //A
	//GC2145MIPI_write_cmos_sensor(0x4d , 0x6a);
	//GC2145MIPI_write_cmos_sensor(0x4e , 0x06);
	
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02); 
	GC2145MIPI_write_cmos_sensor(0x4d , 0x8a);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x06);
	                
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x49);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x06);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x69);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x06);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x89);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x06);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xa9);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x06);
	               
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x48);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x06);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x68);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x06);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x69);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x06);
	             
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);//H
	GC2145MIPI_write_cmos_sensor(0x4d , 0xca);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x07);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xc9);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x07);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xe9);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x07);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x03);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x09);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x07);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xc8);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x07);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xe8);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x07);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xa7);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x07);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xc7);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x07);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x02);
	GC2145MIPI_write_cmos_sensor(0x4d , 0xe7);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x07);
	GC2145MIPI_write_cmos_sensor(0x4c , 0x03);
	GC2145MIPI_write_cmos_sensor(0x4d , 0x07);
	GC2145MIPI_write_cmos_sensor(0x4e , 0x07);
	
	GC2145MIPI_write_cmos_sensor(0x4f , 0x01);
	GC2145MIPI_write_cmos_sensor(0x50 , 0x80);
	GC2145MIPI_write_cmos_sensor(0x51 , 0xa8);
	GC2145MIPI_write_cmos_sensor(0x52 , 0x47);
	GC2145MIPI_write_cmos_sensor(0x53 , 0x38);
	GC2145MIPI_write_cmos_sensor(0x54 , 0xc7);
	GC2145MIPI_write_cmos_sensor(0x56 , 0x0e);
	GC2145MIPI_write_cmos_sensor(0x58 , 0x08);
	GC2145MIPI_write_cmos_sensor(0x5b , 0x00);
	GC2145MIPI_write_cmos_sensor(0x5c , 0x74);
	GC2145MIPI_write_cmos_sensor(0x5d , 0x8b);
	GC2145MIPI_write_cmos_sensor(0x61 , 0xdb);
	GC2145MIPI_write_cmos_sensor(0x62 , 0xb8);
	GC2145MIPI_write_cmos_sensor(0x63 , 0x86);
	GC2145MIPI_write_cmos_sensor(0x64 , 0xc0);
	GC2145MIPI_write_cmos_sensor(0x65 , 0x04);
	
	GC2145MIPI_write_cmos_sensor(0x67 , 0xa8);
	GC2145MIPI_write_cmos_sensor(0x68 , 0xb0);
	GC2145MIPI_write_cmos_sensor(0x69 , 0x00);
	GC2145MIPI_write_cmos_sensor(0x6a , 0xa8);
	GC2145MIPI_write_cmos_sensor(0x6b , 0xb0);
	GC2145MIPI_write_cmos_sensor(0x6c , 0xaf);
	GC2145MIPI_write_cmos_sensor(0x6d , 0x8b);
	GC2145MIPI_write_cmos_sensor(0x6e , 0x50);
	GC2145MIPI_write_cmos_sensor(0x6f , 0x18);
	GC2145MIPI_write_cmos_sensor(0x73 , 0xf0);
	GC2145MIPI_write_cmos_sensor(0x70 , 0x0d);
	GC2145MIPI_write_cmos_sensor(0x71 , 0x60);
	GC2145MIPI_write_cmos_sensor(0x72 , 0x80);
	GC2145MIPI_write_cmos_sensor(0x74 , 0x01);
	GC2145MIPI_write_cmos_sensor(0x75 , 0x01);
	GC2145MIPI_write_cmos_sensor(0x7f , 0x0c);
	GC2145MIPI_write_cmos_sensor(0x76 , 0x70);
	GC2145MIPI_write_cmos_sensor(0x77 , 0x58);
	GC2145MIPI_write_cmos_sensor(0x78 , 0xa0);
	GC2145MIPI_write_cmos_sensor(0x79 , 0x5e);
	GC2145MIPI_write_cmos_sensor(0x7a , 0x54);
	GC2145MIPI_write_cmos_sensor(0x7b , 0x58);                                      
	GC2145MIPI_write_cmos_sensor(0xfe , 0x00);
	//////////////////////////////////////////
	///////////CC////////////////////////
	//////////////////////////////////////////
	GC2145MIPI_write_cmos_sensor(0xfe , 0x02);
	GC2145MIPI_write_cmos_sensor(0xc0 , 0x01);                                   
	GC2145MIPI_write_cmos_sensor(0xc1 , 0x44);
	GC2145MIPI_write_cmos_sensor(0xc2 , 0xfd);
	GC2145MIPI_write_cmos_sensor(0xc3 , 0x04);
	GC2145MIPI_write_cmos_sensor(0xc4 , 0xf0);
	GC2145MIPI_write_cmos_sensor(0xc5 , 0x48);
	GC2145MIPI_write_cmos_sensor(0xc6 , 0xfd);
	GC2145MIPI_write_cmos_sensor(0xc7 , 0x46);
	GC2145MIPI_write_cmos_sensor(0xc8 , 0xfd);
	GC2145MIPI_write_cmos_sensor(0xc9 , 0x02);
	GC2145MIPI_write_cmos_sensor(0xca , 0xe0);
	GC2145MIPI_write_cmos_sensor(0xcb , 0x45);
	GC2145MIPI_write_cmos_sensor(0xcc , 0xec);                         
	GC2145MIPI_write_cmos_sensor(0xcd , 0x48);
	GC2145MIPI_write_cmos_sensor(0xce , 0xf0);
	GC2145MIPI_write_cmos_sensor(0xcf , 0xf0);
	GC2145MIPI_write_cmos_sensor(0xe3 , 0x0c);
	GC2145MIPI_write_cmos_sensor(0xe4 , 0x4b);
	GC2145MIPI_write_cmos_sensor(0xe5 , 0xe0);
	//////////////////////////////////////////
	///////////ABS ////////////////////
	//////////////////////////////////////////
	GC2145MIPI_write_cmos_sensor(0xfe , 0x01);
	GC2145MIPI_write_cmos_sensor(0x9f , 0x40);
	GC2145MIPI_write_cmos_sensor(0xfe , 0x00); 
	//////////////////////////////////////
	///////////  OUTPUT   ////////////////
	//////////////////////////////////////
	GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
	GC2145MIPI_write_cmos_sensor(0xf2, 0x00);
	
	//////////////frame rate 50Hz/////////
	GC2145MIPI_write_cmos_sensor(0xfe , 0x00);
	GC2145MIPI_write_cmos_sensor(0x05 , 0x01);
	GC2145MIPI_write_cmos_sensor(0x06 , 0x56);
	GC2145MIPI_write_cmos_sensor(0x07 , 0x00);
	GC2145MIPI_write_cmos_sensor(0x08 , 0x32);
	GC2145MIPI_write_cmos_sensor(0xfe , 0x01);
	GC2145MIPI_write_cmos_sensor(0x25 , 0x00);
	GC2145MIPI_write_cmos_sensor(0x26 , 0xfa); 
	GC2145MIPI_write_cmos_sensor(0x27 , 0x04); 
	GC2145MIPI_write_cmos_sensor(0x28 , 0xe2); //20fps 
	GC2145MIPI_write_cmos_sensor(0x29 , 0x06); 
	GC2145MIPI_write_cmos_sensor(0x2a , 0xd6); //14fps 
	GC2145MIPI_write_cmos_sensor(0x2b , 0x07); 
	GC2145MIPI_write_cmos_sensor(0x2c , 0xd0); //12fps
	GC2145MIPI_write_cmos_sensor(0x2d , 0x0b); 
	GC2145MIPI_write_cmos_sensor(0x2e , 0xb8); //8fps
	GC2145MIPI_write_cmos_sensor(0xfe , 0x00);
	
	///////////////dark sun////////////////////
	GC2145MIPI_write_cmos_sensor(0xfe , 0x02);
	GC2145MIPI_write_cmos_sensor(0x40 , 0xbf);
	GC2145MIPI_write_cmos_sensor(0x46 , 0xcf);
	GC2145MIPI_write_cmos_sensor(0xfe , 0x00);
	/////////////////////////////////////////////////////
	//////////////////////   MIPI   /////////////////////
	/////////////////////////////////////////////////////
	GC2145MIPI_write_cmos_sensor(0xfe, 0x03);
	GC2145MIPI_write_cmos_sensor(0x02, 0x22);
	GC2145MIPI_write_cmos_sensor(0x03, 0x10); // 0x12 20140821
	GC2145MIPI_write_cmos_sensor(0x04, 0x10); // 0x01 
	GC2145MIPI_write_cmos_sensor(0x05, 0x00);
	GC2145MIPI_write_cmos_sensor(0x06, 0x88);
	#ifdef GC2145MIPI_2Lane
		GC2145MIPI_write_cmos_sensor(0x01, 0x87);
		GC2145MIPI_write_cmos_sensor(0x10, 0x95);
	#else
		GC2145MIPI_write_cmos_sensor(0x01, 0x83);
		GC2145MIPI_write_cmos_sensor(0x10, 0x94);
	#endif
	GC2145MIPI_write_cmos_sensor(0x11, 0x1e);
	GC2145MIPI_write_cmos_sensor(0x12, 0x80);
	GC2145MIPI_write_cmos_sensor(0x13, 0x0c);
	GC2145MIPI_write_cmos_sensor(0x15, 0x10);
	GC2145MIPI_write_cmos_sensor(0x17, 0xf0);
	
	GC2145MIPI_write_cmos_sensor(0x21, 0x10);
	GC2145MIPI_write_cmos_sensor(0x22, 0x04);
	GC2145MIPI_write_cmos_sensor(0x23, 0x10);
	GC2145MIPI_write_cmos_sensor(0x24, 0x10);
	GC2145MIPI_write_cmos_sensor(0x25, 0x10);
	GC2145MIPI_write_cmos_sensor(0x26, 0x05);
	GC2145MIPI_write_cmos_sensor(0x29, 0x03);
	GC2145MIPI_write_cmos_sensor(0x2a, 0x0a);
	GC2145MIPI_write_cmos_sensor(0x2b, 0x06);
	GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
	
}


static void GC2145MIPI_Sensor_SVGA(void)
{
	SENSORDB("GC2145MIPI_Sensor_SVGA");
  
	GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
	GC2145MIPI_write_cmos_sensor(0xfd, 0x01);
	GC2145MIPI_write_cmos_sensor(0xfa, 0x00);
	//// crop window             
	GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
	GC2145MIPI_write_cmos_sensor(0x90, 0x01);
	GC2145MIPI_write_cmos_sensor(0x91, 0x00);
	GC2145MIPI_write_cmos_sensor(0x92, 0x00);
	GC2145MIPI_write_cmos_sensor(0x93, 0x00);
	GC2145MIPI_write_cmos_sensor(0x94, 0x00);
	GC2145MIPI_write_cmos_sensor(0x95, 0x02);
	GC2145MIPI_write_cmos_sensor(0x96, 0x58);
	GC2145MIPI_write_cmos_sensor(0x97, 0x03);
	GC2145MIPI_write_cmos_sensor(0x98, 0x20);
	GC2145MIPI_write_cmos_sensor(0x99, 0x11);
	GC2145MIPI_write_cmos_sensor(0x9a, 0x06);
	//// AWB                     
	GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
	GC2145MIPI_write_cmos_sensor(0xec, 0x02);
	GC2145MIPI_write_cmos_sensor(0xed, 0x02);
	GC2145MIPI_write_cmos_sensor(0xee, 0x30);
	GC2145MIPI_write_cmos_sensor(0xef, 0x48);
	GC2145MIPI_write_cmos_sensor(0xfe, 0x02);
	GC2145MIPI_write_cmos_sensor(0x9d, 0x08);
	GC2145MIPI_write_cmos_sensor(0xfe, 0x01);
	GC2145MIPI_write_cmos_sensor(0x74, 0x00);
	//// AEC                     
	GC2145MIPI_write_cmos_sensor(0xfe, 0x01);
	GC2145MIPI_write_cmos_sensor(0x01, 0x04);
	GC2145MIPI_write_cmos_sensor(0x02, 0x60);
	GC2145MIPI_write_cmos_sensor(0x03, 0x02);
	GC2145MIPI_write_cmos_sensor(0x04, 0x48);
	GC2145MIPI_write_cmos_sensor(0x05, 0x18);
	GC2145MIPI_write_cmos_sensor(0x06, 0x50);
	GC2145MIPI_write_cmos_sensor(0x07, 0x10);
	GC2145MIPI_write_cmos_sensor(0x08, 0x38);
	GC2145MIPI_write_cmos_sensor(0x0a, 0x80);
	GC2145MIPI_write_cmos_sensor(0x21, 0x04);
	GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
	GC2145MIPI_write_cmos_sensor(0x20, 0x03);
	//// mipi
	GC2145MIPI_write_cmos_sensor(0xfe, 0x03);
	GC2145MIPI_write_cmos_sensor(0x12, 0x40);
	GC2145MIPI_write_cmos_sensor(0x13, 0x06);
#if defined(GC2145MIPI_2Lane)
	GC2145MIPI_write_cmos_sensor(0x04, 0x90);
	GC2145MIPI_write_cmos_sensor(0x05, 0x01);
#else
	GC2145MIPI_write_cmos_sensor(0x04, 0x01);
	GC2145MIPI_write_cmos_sensor(0x05, 0x00);
#endif
	GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
}

static void GC2145MIPI_Sensor_2M(void)
{
	SENSORDB("GC2145MIPI_Sensor_2M");
	GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
	GC2145MIPI_write_cmos_sensor(0xfd, 0x00); 
#ifdef GC2145MIPI_2Lane
	GC2145MIPI_write_cmos_sensor(0xfa, 0x00); 
#else
	GC2145MIPI_write_cmos_sensor(0xfa, 0x11); 
#endif
	//// crop window           
	GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
	GC2145MIPI_write_cmos_sensor(0x90, 0x01); 
	GC2145MIPI_write_cmos_sensor(0x91, 0x00);
	GC2145MIPI_write_cmos_sensor(0x92, 0x00);
	GC2145MIPI_write_cmos_sensor(0x93, 0x00);
	GC2145MIPI_write_cmos_sensor(0x94, 0x00);
	GC2145MIPI_write_cmos_sensor(0x95, 0x04);
	GC2145MIPI_write_cmos_sensor(0x96, 0xb0);
	GC2145MIPI_write_cmos_sensor(0x97, 0x06);
	GC2145MIPI_write_cmos_sensor(0x98, 0x40);
	GC2145MIPI_write_cmos_sensor(0x99, 0x11); 
	GC2145MIPI_write_cmos_sensor(0x9a, 0x06);
	//// AWB                   
	GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
	GC2145MIPI_write_cmos_sensor(0xec, 0x06); 
	GC2145MIPI_write_cmos_sensor(0xed, 0x04);
	GC2145MIPI_write_cmos_sensor(0xee, 0x60);
	GC2145MIPI_write_cmos_sensor(0xef, 0x90);
	GC2145MIPI_write_cmos_sensor(0xfe, 0x01);
	GC2145MIPI_write_cmos_sensor(0x74, 0x01); 
	//// AEC                    
	GC2145MIPI_write_cmos_sensor(0xfe, 0x01);
	GC2145MIPI_write_cmos_sensor(0x01, 0x04);
	GC2145MIPI_write_cmos_sensor(0x02, 0xc0);
	GC2145MIPI_write_cmos_sensor(0x03, 0x04);
	GC2145MIPI_write_cmos_sensor(0x04, 0x90);
	GC2145MIPI_write_cmos_sensor(0x05, 0x30);
	GC2145MIPI_write_cmos_sensor(0x06, 0x90);
	GC2145MIPI_write_cmos_sensor(0x07, 0x30);
	GC2145MIPI_write_cmos_sensor(0x08, 0x80);
	GC2145MIPI_write_cmos_sensor(0x0a, 0x82);
#ifdef GC2145MIPI_2Lane
	GC2145MIPI_write_cmos_sensor(0xfe, 0x01);
	GC2145MIPI_write_cmos_sensor(0x21, 0x04); 
	GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
	GC2145MIPI_write_cmos_sensor(0x20, 0x03); 
#else
	GC2145MIPI_write_cmos_sensor(0xfe, 0x01);
	GC2145MIPI_write_cmos_sensor(0x21, 0x15); 
	GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
	GC2145MIPI_write_cmos_sensor(0x20, 0x15); 
#endif
	//// mipi
	GC2145MIPI_write_cmos_sensor(0xfe, 0x03);
	GC2145MIPI_write_cmos_sensor(0x12, 0x80);
	GC2145MIPI_write_cmos_sensor(0x13, 0x0c);
	GC2145MIPI_write_cmos_sensor(0x04, 0x01);
	GC2145MIPI_write_cmos_sensor(0x05, 0x00);
	GC2145MIPI_write_cmos_sensor(0xfe, 0x00);

}


/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*	GC2145MIPIOpen
*
* DESCRIPTION
*	This function initialize the registers of CMOS sensor
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 GC2145MIPIOpen(void)
{
	volatile signed char i;
	kal_uint16 sensor_id=0;

	zoom_factor = 0; 
	Sleep(10);


	//  Read sensor ID to adjust I2C is OK?
	for(i=0;i<3;i++)
	{
		sensor_id=((GC2145MIPI_read_cmos_sensor(0xf0) << 8) | GC2145MIPI_read_cmos_sensor(0xf1));   
		SENSORDB("GC2145MIPI_Open, sensor_id:%x \n",sensor_id);
		if (sensor_id != GC2145_SENSOR_ID)
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	
		SENSORDB("GC2145MIPI Sensor Read ID OK \r\n");
		GC2145MIPI_Sensor_Init();

#ifdef DEBUG_SENSOR_GC2145MIPI  
		struct file *fp; 
		mm_segment_t fs; 
		loff_t pos = 0; 
		static char buf[60*1024] ;

		printk("open 2145 debug \n");
		printk("open 2145 debug \n");
		printk("open 2145 debug \n");	


		fp = filp_open("/mnt/sdcard/gc2145mipi_sd.txt", O_RDONLY , 0); 

		if (IS_ERR(fp)) 
		{ 

			fromsd = 0;   
			printk("open 2145 file error\n");
			printk("open 2145 file error\n");
			printk("open 2145 file error\n");		


		} 
		else 
		{
			fromsd = 1;
			printk("open 2145 file ok\n");
			printk("open 2145 file ok\n");
			printk("open 2145 file ok\n");

			//gc2145mipi_Initialize_from_T_Flash();
			
			filp_close(fp, NULL); 
			set_fs(fs);
		}

		if(fromsd == 1)
		{
			printk("________________2145 from t!\n");
			printk("________________2145 from t!\n");
			printk("________________2145 from t!\n");		
			GC2145MIPI_Initialize_from_T_Flash();
			printk("______after_____2145 from t!\n");	
		}
		else
		{
			//GC2145MIPI_MPEG4_encode_mode = KAL_FALSE;
			printk("________________2145 not from t!\n");	
			printk("________________2145 not from t!\n");
			printk("________________2145 not from t!\n");		
			RETAILMSG(1, (TEXT("Sensor Read ID OK \r\n"))); 
		}

#endif
	
	Preview_Shutter =GC2145MIPI_read_shutter();
	
	return ERROR_NONE;
}	/* GC2145MIPIOpen() */

/*************************************************************************
* FUNCTION
*	GC2145MIPIClose
*
* DESCRIPTION
*	This function is to turn off sensor module power.
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 GC2145MIPIClose(void)
{
//	CISModulePowerOn(FALSE);
	return ERROR_NONE;
}	/* GC2145MIPIClose() */

/*************************************************************************
* FUNCTION
*	GC2145MIPIPreview
*
* DESCRIPTION
*	This function start the sensor preview.
*
* PARAMETERS
*	*image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 GC2145MIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint8 iTemp, temp_AE_reg, temp_AWB_reg;
	kal_uint16 iDummyPixels = 0, iDummyLines = 0, iStartX = 0, iStartY = 0;

	SENSORDB("GC2145MIPIPrevie\n");

	GC2145MIPI_sensor_cap_state = KAL_FALSE;


	GC2145MIPI_Sensor_SVGA();
	GC2145MIPI_write_shutter(Preview_Shutter);


	GC2145MIPI_set_AE_mode(KAL_TRUE); 

	Sleep(100);
	memcpy(&GC2145MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	return ERROR_NONE;
}	/* GC2145MIPIPreview() */




UINT32 GC2145MIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    volatile kal_uint32 shutter = GC2145MIPI_exposure_lines, temp_reg;
    kal_uint8 temp_AE_reg, temp;
    kal_uint16 AE_setting_delay = 0;

    SENSORDB("GC2145MIPICapture\n");

  if(GC2145MIPI_sensor_cap_state == KAL_FALSE)
 	{


#if defined(GC2145MIPI_2Lane)

	GC2145MIPI_Sensor_2M();

#else
	// turn off AEC/AGC
	GC2145MIPI_set_AE_mode(KAL_FALSE);
	
	shutter = GC2145MIPI_read_shutter();
	Preview_Shutter = shutter;
	
	GC2145MIPI_Sensor_2M();
	
	Capture_Shutter = shutter / 2; 
	// set shutter
	GC2145MIPI_write_shutter(Capture_Shutter);
#endif
	Sleep(200);
      }

     GC2145MIPI_sensor_cap_state = KAL_TRUE;

	image_window->GrabStartX=1;
        image_window->GrabStartY=1;
        image_window->ExposureWindowWidth=GC2145MIPI_IMAGE_SENSOR_FULL_WIDTH - image_window->GrabStartX;
        image_window->ExposureWindowHeight=GC2145MIPI_IMAGE_SENSOR_FULL_HEIGHT -image_window->GrabStartY;    	 

    memcpy(&GC2145MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	return ERROR_NONE;
}	/* GC2145MIPICapture() */



UINT32 GC2145MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	pSensorResolution->SensorFullWidth=GC2145MIPI_IMAGE_SENSOR_FULL_WIDTH - 2 * IMAGE_SENSOR_START_GRAB_X;
	pSensorResolution->SensorFullHeight=GC2145MIPI_IMAGE_SENSOR_FULL_HEIGHT - 2 * IMAGE_SENSOR_START_GRAB_Y;
	pSensorResolution->SensorPreviewWidth=GC2145MIPI_IMAGE_SENSOR_PV_WIDTH - 2 * IMAGE_SENSOR_START_GRAB_X;
	pSensorResolution->SensorPreviewHeight=GC2145MIPI_IMAGE_SENSOR_PV_HEIGHT - 2 * IMAGE_SENSOR_START_GRAB_Y;
	pSensorResolution->SensorVideoWidth=GC2145MIPI_IMAGE_SENSOR_PV_WIDTH - 2 * IMAGE_SENSOR_START_GRAB_X;
	pSensorResolution->SensorVideoHeight=GC2145MIPI_IMAGE_SENSOR_PV_HEIGHT - 2 * IMAGE_SENSOR_START_GRAB_Y;
	
	pSensorResolution->SensorHighSpeedVideoWidth=GC2145MIPI_IMAGE_SENSOR_PV_WIDTH - 2 * IMAGE_SENSOR_START_GRAB_X;
    pSensorResolution->SensorHighSpeedVideoHeight=GC2145MIPI_IMAGE_SENSOR_PV_HEIGHT - 2 * IMAGE_SENSOR_START_GRAB_Y;
    
    pSensorResolution->SensorSlimVideoWidth=GC2145MIPI_IMAGE_SENSOR_PV_WIDTH - 2 * IMAGE_SENSOR_START_GRAB_X;
    pSensorResolution->SensorSlimVideoHeight=GC2145MIPI_IMAGE_SENSOR_PV_HEIGHT - 2 * IMAGE_SENSOR_START_GRAB_Y; 
    
	return ERROR_NONE;
}	/* GC2145MIPIGetResolution() */

UINT32 GC2145MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{

	pSensorInfo->SensorPreviewResolutionX=GC2145MIPI_IMAGE_SENSOR_PV_WIDTH;
	pSensorInfo->SensorPreviewResolutionY=GC2145MIPI_IMAGE_SENSOR_PV_HEIGHT;
	pSensorInfo->SensorFullResolutionX=GC2145MIPI_IMAGE_SENSOR_FULL_WIDTH;
	pSensorInfo->SensorFullResolutionY=GC2145MIPI_IMAGE_SENSOR_FULL_HEIGHT;

	pSensorInfo->SensorCameraPreviewFrameRate=30;
	pSensorInfo->SensorVideoFrameRate=30;
	pSensorInfo->SensorStillCaptureFrameRate=10;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=FALSE;
	pSensorInfo->SensorResetDelayCount=1;
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV;
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	/*??? */
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorInterruptDelayLines = 1;
	pSensorInfo->CaptureDelayFrame = 4; 
	pSensorInfo->PreviewDelayFrame = 2; 
	pSensorInfo->VideoDelayFrame = 0; 
	pSensorInfo->HighSpeedVideoDelayFrame = 0;
    pSensorInfo->SlimVideoDelayFrame = 0;
    pSensorInfo->SensorModeNum = 5;
    
    pSensorInfo->YUVAwbDelayFrame = 2;  // add by lanking
	pSensorInfo->YUVEffectDelayFrame = 2;  // add by lanking
	pSensorInfo->SensorMasterClockSwitch = 0; 
	pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_6MA;


	pSensorInfo->SensroInterfaceType = SENSOR_INTERFACE_TYPE_MIPI;

	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
		default:

			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
                     pSensorInfo->SensorGrabStartX = 2; 
                     pSensorInfo->SensorGrabStartY = 2;
		#ifdef GC2145MIPI_2Lane
			pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;		
		#else
			pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;
		#endif
			pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
			pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
			pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
			pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
			pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
			pSensorInfo->SensorPacketECCOrder = 1;

	
		break;
	
	}
	memcpy(pSensorConfigData, &GC2145MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	return ERROR_NONE;
}	/* GC2145MIPIGetInfo() */


UINT32 GC2145MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			GC2145MIPIPreview(pImageWindow, pSensorConfigData);
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			GC2145MIPICapture(pImageWindow, pSensorConfigData);
		break;
		default:
		    break; 
	}
	return TRUE;
}	/* GC2145MIPIControl() */

BOOL GC2145MIPI_set_param_wb(UINT16 para)
{
	switch (para)
	{
		case AWB_MODE_AUTO:
			GC2145MIPI_set_AWB_mode(KAL_TRUE);
		break;
		case AWB_MODE_CLOUDY_DAYLIGHT: //cloudy
			GC2145MIPI_set_AWB_mode(KAL_FALSE);
			GC2145MIPI_write_cmos_sensor(0xb3, 0x58);
			GC2145MIPI_write_cmos_sensor(0xb4, 0x40);
			GC2145MIPI_write_cmos_sensor(0xb5, 0x50);
		break;
		case AWB_MODE_DAYLIGHT: //sunny
			GC2145MIPI_set_AWB_mode(KAL_FALSE);
			GC2145MIPI_write_cmos_sensor(0xb3, 0x70);
			GC2145MIPI_write_cmos_sensor(0xb4, 0x40);
			GC2145MIPI_write_cmos_sensor(0xb5, 0x50);
		break;
		case AWB_MODE_INCANDESCENT: //office
			GC2145MIPI_set_AWB_mode(KAL_FALSE);
			GC2145MIPI_write_cmos_sensor(0xb3, 0x50);
			GC2145MIPI_write_cmos_sensor(0xb4, 0x40);
			GC2145MIPI_write_cmos_sensor(0xb5, 0xa8);
		break;
		case AWB_MODE_TUNGSTEN: //home
			GC2145MIPI_set_AWB_mode(KAL_FALSE);
			GC2145MIPI_write_cmos_sensor(0xb3, 0xa0);
			GC2145MIPI_write_cmos_sensor(0xb4, 0x45);
			GC2145MIPI_write_cmos_sensor(0xb5, 0x40);
		break;
		case AWB_MODE_FLUORESCENT:
			GC2145MIPI_set_AWB_mode(KAL_FALSE);
			GC2145MIPI_write_cmos_sensor(0xb3, 0x72);
			GC2145MIPI_write_cmos_sensor(0xb4, 0x40);
			GC2145MIPI_write_cmos_sensor(0xb5, 0x5b);
		break;	
		default:
		return FALSE;
	}
	return TRUE;
} /* GC2145MIPI_set_param_wb */

BOOL GC2145MIPI_set_param_effect(UINT16 para)
{
	kal_uint32 ret = KAL_TRUE;
	switch (para)
	{
		case MEFFECT_OFF:
			GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
			GC2145MIPI_write_cmos_sensor(0x83, 0xe0);
		break;

		case MEFFECT_SEPIA:
			GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
			GC2145MIPI_write_cmos_sensor(0x83, 0x82);
		break;  

		case MEFFECT_NEGATIVE:		
			GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
			GC2145MIPI_write_cmos_sensor(0x83, 0x01);
		break; 

		case MEFFECT_SEPIAGREEN:		
			GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
			GC2145MIPI_write_cmos_sensor(0x83, 0x52);
		break;

		case MEFFECT_SEPIABLUE:	
			GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
			GC2145MIPI_write_cmos_sensor(0x83, 0x62);
		break;

		case MEFFECT_MONO:				
			GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
			GC2145MIPI_write_cmos_sensor(0x83, 0x12);
		break;

		default:
		return FALSE;
	}

	return ret;
} /* GC2145MIPI_set_param_effect */

BOOL GC2145MIPI_set_param_banding(UINT16 para)
{
    switch (para)
    {
        case AE_FLICKER_MODE_50HZ:
			
		GC2145MIPI_write_cmos_sensor(0xfe , 0x00);
		GC2145MIPI_write_cmos_sensor(0x05 , 0x01);
		GC2145MIPI_write_cmos_sensor(0x06 , 0x56);
		GC2145MIPI_write_cmos_sensor(0x07 , 0x00);
		GC2145MIPI_write_cmos_sensor(0x08 , 0x32);
		GC2145MIPI_write_cmos_sensor(0xfe , 0x01);
		GC2145MIPI_write_cmos_sensor(0x25 , 0x00);
		GC2145MIPI_write_cmos_sensor(0x26 , 0xfa); 
		GC2145MIPI_write_cmos_sensor(0x27 , 0x04); 
		GC2145MIPI_write_cmos_sensor(0x28 , 0xe2); //20fps 
		GC2145MIPI_write_cmos_sensor(0x29 , 0x06); 
		GC2145MIPI_write_cmos_sensor(0x2a , 0xd6); //14fps 
		GC2145MIPI_write_cmos_sensor(0x2b , 0x07); 
		GC2145MIPI_write_cmos_sensor(0x2c , 0xd0); //12fps 
		GC2145MIPI_write_cmos_sensor(0x2d , 0x0b); 
		GC2145MIPI_write_cmos_sensor(0x2e , 0xb8); //8fps
		GC2145MIPI_write_cmos_sensor(0xfe , 0x00);
            break;

        case AE_FLICKER_MODE_60HZ:
		GC2145MIPI_write_cmos_sensor(0xfe , 0x00);
		GC2145MIPI_write_cmos_sensor(0x05 , 0x01);
		GC2145MIPI_write_cmos_sensor(0x06 , 0x58);
		GC2145MIPI_write_cmos_sensor(0x07 , 0x00);
		GC2145MIPI_write_cmos_sensor(0x08 , 0x32);
		GC2145MIPI_write_cmos_sensor(0xfe , 0x01);
		GC2145MIPI_write_cmos_sensor(0x25 , 0x00);
		GC2145MIPI_write_cmos_sensor(0x26 , 0xd0); 
		GC2145MIPI_write_cmos_sensor(0x27 , 0x04); 
		GC2145MIPI_write_cmos_sensor(0x28 , 0xe0); //20fps 
		GC2145MIPI_write_cmos_sensor(0x29 , 0x06); 
		GC2145MIPI_write_cmos_sensor(0x2a , 0x80); //14fps 
		GC2145MIPI_write_cmos_sensor(0x2b , 0x08); 
		GC2145MIPI_write_cmos_sensor(0x2c , 0x20); //12fps 
		GC2145MIPI_write_cmos_sensor(0x2d , 0x0b); 
		GC2145MIPI_write_cmos_sensor(0x2e , 0x60); //8fps
		GC2145MIPI_write_cmos_sensor(0xfe , 0x00);
            break;

          default:
              return FALSE;
    }

    return TRUE;
} /* GC2145MIPI_set_param_banding */

BOOL GC2145MIPI_set_param_exposure(UINT16 para)
{
	switch (para)
	{
		
		case AE_EV_COMP_n30:
			GC2145MIPI_SET_PAGE1;
			GC2145MIPI_write_cmos_sensor(0x13,0x65);
			GC2145MIPI_SET_PAGE0;
		break;
		case AE_EV_COMP_n20:
			GC2145MIPI_SET_PAGE1;
			GC2145MIPI_write_cmos_sensor(0x13,0x70);
			GC2145MIPI_SET_PAGE0;
		break;
		case AE_EV_COMP_n10:
			GC2145MIPI_SET_PAGE1;
			GC2145MIPI_write_cmos_sensor(0x13,0x75);
			GC2145MIPI_SET_PAGE0;
		break;
		case AE_EV_COMP_00:
			GC2145MIPI_SET_PAGE1;
			GC2145MIPI_write_cmos_sensor(0x13,0x7b);
			GC2145MIPI_SET_PAGE0;
		break;
		case AE_EV_COMP_10:
			GC2145MIPI_SET_PAGE1;
			GC2145MIPI_write_cmos_sensor(0x13,0x85);
			GC2145MIPI_SET_PAGE0;
		break;
		case AE_EV_COMP_20:
			GC2145MIPI_SET_PAGE1;
			GC2145MIPI_write_cmos_sensor(0x13,0x90);
			GC2145MIPI_SET_PAGE0;
		break;
		case AE_EV_COMP_30:
			GC2145MIPI_SET_PAGE1;
			GC2145MIPI_write_cmos_sensor(0x13,0x95);
			GC2145MIPI_SET_PAGE0;
		break;
		default:
		return FALSE;
	}
	return TRUE;
} /* GC2145MIPI_set_param_exposure */

UINT32 GC2145MIPIYUVSensorSetting(FEATURE_ID iCmd, UINT16 iPara)
{
//   if( GC2145MIPI_sensor_cap_state == KAL_TRUE)
//	   return TRUE;

#ifdef DEBUG_SENSOR_GC2145MIPI
		printk("______%s______GC2145MIPI YUV setting\n",__func__);
		return TRUE;
#endif

	switch (iCmd) {
	case FID_SCENE_MODE:	    
//	    printk("Set Scene Mode:%d\n", iPara); 
	    if (iPara == SCENE_MODE_OFF)
	    {
	        GC2145MIPI_night_mode(0); 
	    }
	    else if (iPara == SCENE_MODE_NIGHTSCENE)
	    {
               GC2145MIPI_night_mode(1); 
	    }	    
	    break; 	    
	case FID_AWB_MODE:
	    printk("Set AWB Mode:%d\n", iPara); 	    
           GC2145MIPI_set_param_wb(iPara);
	break;
	case FID_COLOR_EFFECT:
	    printk("Set Color Effect:%d\n", iPara); 	    	    
           GC2145MIPI_set_param_effect(iPara);
	break;
	case FID_AE_EV:
           printk("Set EV:%d\n", iPara); 	    	    
           GC2145MIPI_set_param_exposure(iPara);
	break;
	case FID_AE_FLICKER:
          printk("Set Flicker:%d\n", iPara); 	    	    	    
           GC2145MIPI_set_param_banding(iPara);
	break;
        case FID_AE_SCENE_MODE: 
            if (iPara == AE_MODE_OFF) {
                GC2145MIPI_set_AE_mode(KAL_FALSE);
            }
            else {
                GC2145MIPI_set_AE_mode(KAL_TRUE);
	    }
            break; 
	case FID_ZOOM_FACTOR:
	    zoom_factor = iPara; 
        break; 
	default:
	break;
	}
	return TRUE;
}   /* GC2145MIPIYUVSensorSetting */

UINT32 GC2145MIPIYUVSetVideoMode(uintptr_t u2FrameRate)
{
    kal_uint8 iTemp;
    /* to fix VSYNC, to fix frame rate */
    //printk("Set YUV Video Mode \n");  

    if (u2FrameRate == 30)
    {
    }
    else if (u2FrameRate == 15)       
    {
    }
    else 
    {
        printk("Wrong frame rate setting \n");
    }
    GC2145MIPI_VEDIO_encode_mode = KAL_TRUE; 
        
    return TRUE;
}

/*************************************************************************
  * FUNCTION
  * GC2145SetMaxFramerateByScenario
  *
  * DESCRIPTION
  * This function is for android4.4 kk
  * RETURNS
  * None
  *
  * add by lanking
  *
  *************************************************************************/

  UINT32 GC2145MIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) {
	kal_uint32 pclk;
	kal_int16 dummyLine;
	kal_uint16 lineLength,frameHeight;
		
	SENSORDB("GC2145MIPISetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);
	/*
	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			pclk = 134200000;
			lineLength = IMX111MIPI_PV_LINE_LENGTH_PIXELS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - IMX111MIPI_PV_FRAME_LENGTH_LINES;
			break;
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
			break;		
		default:
			break;
	}	
	*/
	return ERROR_NONE;
}

/*************************************************************************
  * FUNCTION
  * GC2145GetDefaultFramerateByScenario
  *
  * DESCRIPTION
  * This function is for android4.4 kk
  * RETURNS
  * None
  *
  * GLOBALS AFFECTED
  *
  *************************************************************************/
UINT32 GC2145MIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
{

	switch (scenarioId)
	 {
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

 }

 
 UINT32 GC2145SetTestPatternMode(kal_bool bEnable)
 {
     SENSORDB("[GC0310SetTestPatternMode]test pattern bEnable:=%d\n",bEnable);
 
     if(bEnable) 
     {
        GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
        GC2145MIPI_write_cmos_sensor(0x80, 0x08);
        GC2145MIPI_write_cmos_sensor(0x81, 0x00);
        GC2145MIPI_write_cmos_sensor(0x82, 0x00);
        GC2145MIPI_write_cmos_sensor(0xb6, 0x00);

        GC2145MIPI_write_cmos_sensor(0x18, 0x06);

        GC2145MIPI_write_cmos_sensor(0xb3, 0x40);
        GC2145MIPI_write_cmos_sensor(0xb4, 0x40);
        GC2145MIPI_write_cmos_sensor(0xb5, 0x40);

        GC2145MIPI_write_cmos_sensor(0xfe, 0x02);
        GC2145MIPI_write_cmos_sensor(0xd0, 0x40);
        GC2145MIPI_write_cmos_sensor(0xdd, 0x00);

        GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
        GC2145MIPI_write_cmos_sensor(0xb1, 0x40);
        GC2145MIPI_write_cmos_sensor(0xb2, 0x40);
        GC2145MIPI_write_cmos_sensor(0x03, 0x00);
        GC2145MIPI_write_cmos_sensor(0x04, 0x00);
        
		GC2145MIPI_write_cmos_sensor(0x8c, 0x01);
		GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
     }
     else
     {
        GC2145MIPI_write_cmos_sensor(0xfe, 0x00);	
		GC2145MIPI_write_cmos_sensor(0x8c, 0x00);
		GC2145MIPI_write_cmos_sensor(0xfe, 0x00);
     }
     
     return ERROR_NONE;
 }
 
 /*************************************************************************
  * FUNCTION
  * GC2145MIPIFeatureControl
  *
  * DESCRIPTION
  * 
  * RETURNS
  * None
  *
  * GLOBALS AFFECTED
  *
  *************************************************************************/

UINT32 GC2145MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
							 UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
	UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
	UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    unsigned long long *feature_data=(unsigned long long *) pFeaturePara;
    unsigned long long *feature_return_para=(unsigned long long *) pFeaturePara;
	
	MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;

	switch (FeatureId)
	{
		case SENSOR_FEATURE_GET_RESOLUTION:
			*pFeatureReturnPara16++=GC2145MIPI_IMAGE_SENSOR_FULL_WIDTH;
			*pFeatureReturnPara16=GC2145MIPI_IMAGE_SENSOR_FULL_HEIGHT;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_PERIOD:
			*pFeatureReturnPara16++=GC2145MIPI_IMAGE_SENSOR_PV_WIDTH;
			*pFeatureReturnPara16=GC2145MIPI_IMAGE_SENSOR_PV_HEIGHT;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			//*pFeatureReturnPara32 = GC2145MIPI_sensor_pclk/10;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_SET_ESHUTTER:
		break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			GC2145MIPI_night_mode((BOOL) *feature_data);
		break;
		case SENSOR_FEATURE_SET_GAIN:
		case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			GC2145MIPI_isp_master_clock=*pFeatureData32;
		break;
		case SENSOR_FEATURE_SET_REGISTER:
			GC2145MIPI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
		case SENSOR_FEATURE_GET_REGISTER:
			pSensorRegData->RegData = GC2145MIPI_read_cmos_sensor(pSensorRegData->RegAddr);
		break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			memcpy(pSensorConfigData, &GC2145MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
			*pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
		break;
		case SENSOR_FEATURE_SET_CCT_REGISTER:
		case SENSOR_FEATURE_GET_CCT_REGISTER:
		case SENSOR_FEATURE_SET_ENG_REGISTER:
		case SENSOR_FEATURE_GET_ENG_REGISTER:
		case SENSOR_FEATURE_GET_REGISTER_DEFAULT:

		case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
		case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
		case SENSOR_FEATURE_GET_GROUP_INFO:
		case SENSOR_FEATURE_GET_ITEM_INFO:
		case SENSOR_FEATURE_SET_ITEM_INFO:
		case SENSOR_FEATURE_GET_ENG_INFO:
		break;
		case SENSOR_FEATURE_GET_GROUP_COUNT:
                        *pFeatureReturnPara32++=0;
                        *pFeatureParaLen=4;	    
		    break; 
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
			 GC2145MIPI_GetSensorID(pFeatureData32);
			 break;
		case SENSOR_FEATURE_SET_YUV_CMD:
		       //printk("GC2145MIPI YUV sensor Setting:%d, %d \n", *pFeatureData32,  *(pFeatureData32+1));
			GC2145MIPIYUVSensorSetting((FEATURE_ID)*feature_data, *(feature_data+1));
		break;
		case SENSOR_FEATURE_SET_VIDEO_MODE:
		       GC2145MIPIYUVSetVideoMode(*feature_data);
		       break;

        case SENSOR_FEATURE_SET_TEST_PATTERN:			 
		    GC2145SetTestPatternMode((BOOL)*feature_data);			
		break;

        case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		    *pFeatureReturnPara32 = GC2145_TEST_PATTERN_CHECKSUM;
		    *pFeatureParaLen=4;
		break;
        
		default:
			break;			
	}
	return ERROR_NONE;
}	/* GC2145MIPIFeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncGC2145MIPI=
{
	GC2145MIPIOpen,
	GC2145MIPIGetInfo,
	GC2145MIPIGetResolution,
	GC2145MIPIFeatureControl,
	GC2145MIPIControl,
	GC2145MIPIClose
};

UINT32 GC2145MIPI_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncGC2145MIPI;

	return ERROR_NONE;
}	/* SensorInit() */
