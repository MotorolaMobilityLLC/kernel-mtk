#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h> 
#include <asm/atomic.h>
//#include <linux/xlog.h>
//#include <asm/system.h>

//#include "kd_camera_hw.h"
#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "ar1335mipi_ofilm_Sensor.h"

#define PFX "AR1335_ofilm_otp"
#define LOG_INF(format, args...)    pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)

#define i2c_write_id  0x6c
struct OTP_Struct {
    u16 rg_ratio;
    u16 bg_ratio;
    u16 gg_ratio;
    u16 inf;
    u16 macro;
    u8 awb_times;
    u8 af_times;
    u8 lsc_times;   
    u8 module_id;   
};

#define RGr_ratio_Typical 663
#define BGr_ratio_Typical 648
#define GrGb_ratio_Typical 1021

u16 Ofilm_AR1335_OTP_Data[9] = {0};

static kal_uint16 Read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;
	kal_uint16 tmp = 0;

    char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
    iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 2, i2c_write_id);

	tmp = get_byte >> 8;
	get_byte = ((get_byte & 0x00ff) << 8) | tmp; 

    return get_byte;
}
static void Write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
    char pu_send_cmd[4] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para >> 8), (char)(para & 0xFF)};
    iWriteRegI2C(pu_send_cmd, 4, i2c_write_id);
}

static void Select_Type(u16 otp_type)            //type 30 moudule information
{                                                              //type 37 awb data  type 11 lsc data
    //int i = 0,flag_end = 0;
    Write_cmos_sensor(0x301A,0x0001);       //type 31 af data    type 38 checksum data     
    mdelay(10);
    Write_cmos_sensor(0x301A,0x0218); 
    Write_cmos_sensor(0x304C,(otp_type&0xff)<<8);
    Write_cmos_sensor(0x3054,0x0400); 
    Write_cmos_sensor(0x304A,0x0210); 
    #if 0
    do{
		flag_end = (Read_cmos_sensor(0x304A));
		if(flag_end&0x60 == 0x60){
			LOG_INF("AR1335 read otpm successful 0x304A = 0x%x\n",flag_end);
			break;
		}
		mDELAY(10);
		i++;
		LOG_INF("AR1335 read otpm error 0x304A = 0x%x,i=%d\n",flag_end,i);
	}while(i<3);
    #endif
}

bool is_ofilm_camera(void)
{
    u8 vendor_id = Read_cmos_sensor(0x3802);
    Select_Type(0x30);
    LOG_INF("vendor_id = %d",vendor_id);
    if(0x6 == vendor_id)//ofilm = 0x06
    {
        return 1;
    }
    return 0;
}

void AR1335_OTP_AUTO_LOAD_LSC(void)
{
     LOG_INF("TINNO_CAMERA_HAL_DRIVER_LOG.OFILM_AR1335.AUTO_LOAD_LSC\n");
    //kal_uint16 temp;
	Select_Type(0x11);
	// enable sc
//  temp = Read_cmos_sensor(0x3780);  
//  Write_cmos_sensor(0x3780,temp|0x8000);  
    Write_cmos_sensor(0x3780,0x8000); 
}

int Ofilm_AR1335_read_OTP(void)
{
    int awb_ck = 0;
    int af_ck = 0;
    int rg_ratio = 0,bg_ratio = 0,gg_ratio = 0;
    int awb_sum  = 0;
    int af_sum  = 0;
    int macro = 0,inf = 0;
    int wb_flag = Read_cmos_sensor(0x3806);
    int af_flag = Read_cmos_sensor(0x3804);
    struct OTP_Struct AR1335_otp_Struct;
    Select_Type(0x30);//0x30 type
    AR1335_otp_Struct.module_id = Read_cmos_sensor(0x3802) &0xff;
    LOG_INF("module_id = %d",AR1335_otp_Struct.module_id);
     Select_Type(0x38); //0x38 type
    awb_ck = Read_cmos_sensor(0x3800);
    af_ck = Read_cmos_sensor(0x3808);
    AR1335_otp_Struct.awb_times = (u8)Read_cmos_sensor(0x3802);
    AR1335_otp_Struct.lsc_times = (u8)Read_cmos_sensor(0x3806);
    AR1335_otp_Struct.af_times = (u8)Read_cmos_sensor(0x380A);
    
    //---------------------------AWB----------------------------------
     Select_Type(0x37); //awb 0x37 type
    
    if(wb_flag != 0xffff){
        LOG_INF("[AR1335_OTP] read awb flag fail!");
        return 0;
    }
    else{
        rg_ratio = Read_cmos_sensor(0x3800);
        bg_ratio = Read_cmos_sensor(0x3802);
        gg_ratio = Read_cmos_sensor(0x3804);
    }
    awb_sum = (rg_ratio + bg_ratio + gg_ratio + wb_flag) % 65535 + 1;
    if(awb_sum != awb_ck)//awb checksum 
    { 
        LOG_INF("[AR1335_OTP] checksum of awb fail!");
        AR1335_otp_Struct.rg_ratio = 0;
        AR1335_otp_Struct.bg_ratio = 0;
        AR1335_otp_Struct.gg_ratio = 0;
        return 0; 
     } 
    else{
        AR1335_otp_Struct.rg_ratio = rg_ratio;
        AR1335_otp_Struct.bg_ratio = bg_ratio;
        AR1335_otp_Struct.gg_ratio = gg_ratio;
        LOG_INF("[AR1335_OTP] ");       
    }
    LOG_INF("rg_ratio = %d,bg_ratio = %d,gg_ratio = %d",rg_ratio,bg_ratio,gg_ratio);
    //---------------------------AF----------------------------------
    Select_Type(0x31);//type 31 af data
    //int af_flag = Read_cmos_sensor(0x3804); 
    if(af_flag != 0xffff)
    {
        LOG_INF("[AR1335_OTP] read af flag fail!");
        return 0;
    }
    else{
         inf=  Read_cmos_sensor(0x3800);
        macro     = Read_cmos_sensor(0x3802);      
    }
   af_sum = (macro + inf + af_flag) % 65535 + 1;
   if(af_sum != af_ck){
        LOG_INF("[AR1335_OTP] checksum of af fail!");
        AR1335_otp_Struct.macro = 0;
        AR1335_otp_Struct.inf = 0;
        return 0;
   }
   else{
     AR1335_otp_Struct.macro = macro;
      AR1335_otp_Struct.inf = inf;
   }
    LOG_INF("macro = %d,inf = %d",macro,inf);
    LOG_INF("TINNO_CAMERA_HAL_DRIVER_LOG_2A_OTP_DATA.OFILM_AR1335.rg_ratio =%d,bg_ratio=%d,gg_ratio =%d\n",AR1335_otp_Struct.rg_ratio,AR1335_otp_Struct.bg_ratio,AR1335_otp_Struct.gg_ratio);
    LOG_INF("TINNO_CAMERA_HAL_DRIVER_LOG_2A_OTP_DATA.OFILM_AR1335.RGr_ratio_Typical =%d,BGr_ratio_Typical=%d,GrGb_ratio_Typical =%d\n",RGr_ratio_Typical,BGr_ratio_Typical,GrGb_ratio_Typical);
    LOG_INF("TINNO_CAMERA_HAL_DRIVER_LOG_2A_OTP_DATA.OFILM_AR1335.inf = %d,macro = %d\n",AR1335_otp_Struct.inf,AR1335_otp_Struct.macro);
    Ofilm_AR1335_OTP_Data[0] = AR1335_otp_Struct.module_id;
   Ofilm_AR1335_OTP_Data[1] = AR1335_otp_Struct.rg_ratio;
   Ofilm_AR1335_OTP_Data[2] = AR1335_otp_Struct.bg_ratio;
   Ofilm_AR1335_OTP_Data[3] = AR1335_otp_Struct.gg_ratio;
   Ofilm_AR1335_OTP_Data[4] = RGr_ratio_Typical;
   Ofilm_AR1335_OTP_Data[5] = BGr_ratio_Typical;
   Ofilm_AR1335_OTP_Data[6] = GrGb_ratio_Typical;
   Ofilm_AR1335_OTP_Data[7] = AR1335_otp_Struct.inf;
   Ofilm_AR1335_OTP_Data[8] = AR1335_otp_Struct.macro;
   return 1;
}



void ofilm_otp_test(void)
{
    u8 module_id = 0;
    int rg_ratio = 0,bg_ratio = 0,gg_ratio = 0;
        int macro = 0,inf = 0;
     Write_cmos_sensor(0x301A,0x0001);       //type 31 af data    type 38 checksum data     
     mdelay(10); 
    Write_cmos_sensor(0x301A,0x0218); 
    Write_cmos_sensor(0x304C,0x3000);
    Write_cmos_sensor(0x3054,0x0400); 
    Write_cmos_sensor(0x304A,0x0210); 

    module_id = Read_cmos_sensor(0x3802);
    LOG_INF("module_id = %d",module_id);

    Write_cmos_sensor(0x301A,0x0001);       //type 31 af data    type 38 checksum data     
     mdelay(10); 
    Write_cmos_sensor(0x301A,0x0218); 
    Write_cmos_sensor(0x304C,0x3700);
    Write_cmos_sensor(0x3054,0x0400); 
    Write_cmos_sensor(0x304A,0x0210); 
            rg_ratio = Read_cmos_sensor(0x3800);
        bg_ratio = Read_cmos_sensor(0x3802);
        gg_ratio = Read_cmos_sensor(0x3804);
        LOG_INF("rg_ratio = %d,bg_ratio = %d,gg_ratio = %d",rg_ratio,bg_ratio,gg_ratio);

            Write_cmos_sensor(0x301A,0x0001);       //type 31 af data    type 38 checksum data     
     mdelay(10); 
    Write_cmos_sensor(0x301A,0x0218); 
    Write_cmos_sensor(0x304C,0x3100);
    Write_cmos_sensor(0x3054,0x0400); 
    Write_cmos_sensor(0x304A,0x0210);

             macro =  Read_cmos_sensor(0x3800);
        inf     = Read_cmos_sensor(0x3802);     

        LOG_INF("macro = %d,inf = %d",macro,inf);
    
    
}




















