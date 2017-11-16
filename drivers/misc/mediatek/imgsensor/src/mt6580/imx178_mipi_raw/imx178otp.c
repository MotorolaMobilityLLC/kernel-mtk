/*===========================================================================

                        EDIT HISTORY FOR V11

when              comment tag        who                  what, where, why                           
----------    ------------     -----------      --------------------------      

===========================================================================*/
/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
*/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
	
#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "imx178mipiraw_Sensor.h"
#include "imx178mipiraw_Camera_Sensor_para.h"
#include "imx178mipiraw_CameraCustomized.h"

#undef CDBG
#define CDBG(fmt, args...) printk(KERN_INFO "imx178_OTP.c: " fmt, ## args)

#define IMX178_OTP_DEBUG

extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
#define IMX178MIPI_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1, IMX178MIPI_WRITE_ID)

extern kal_uint16 IMX178MIPI_read_cmos_sensor(kal_uint32 addr);

static  int  customer_id=0; 
#define custom_TRULY  2
	
struct otp_struct {
    uint customer_id;
    uint year;
    uint month;
    uint day;
    uint lens_id;
    uint vcm_id;
    uint32_t rg_ratio;
    uint32_t bg_ratio;
    uint32_t gg_ratio;
    uint32_t lenc[504];
};

static struct otp_struct current_otp2;
static uint32_t GAIN_DEFAULT=0x100;
static uint32_t R_gain1=0x100,B_gain1=0x100,G_gain1=0x100,G_gain1_R=0x100,G_gain1_B=0x100;
static uint wb_flag=0, lenc_flag=0;

static struct otp_struct current_otp; 

// index: index of otp group. (0, 1, 2)
// return: 0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
static int check_otp_awb(uint index)
{
    uint flag, page, temp;
    uint check_time =10; 

    // select page
    page = 0x00 + index;    
    IMX178MIPI_write_cmos_sensor(0x3402, page);
    // turn on otp read mode
    IMX178MIPI_write_cmos_sensor(0x3400, 0x01);
    // Status check : 0x3B01 = 0x01 (bit0 1:read ready)

   while(check_time--)
   {
	    temp = IMX178MIPI_read_cmos_sensor(0x3401);
	    if ((temp&0x01) ==0x1)
	    {
	     #ifdef IMX178_OTP_DEBUG
	        printk(" check_otp_awb:Status check : 0x3401 == 0x01, read  ready (bit0 1:read  ready)\n");
	      #endif
		  break;
	    }	
   }

   if(check_time == 0)
   	 printk("check_otp_awb:Status check wait:  check_time=%d",check_time);

   
    // read flag
    flag = IMX178MIPI_read_cmos_sensor(0x3404);
#ifdef IMX178_OTP_DEBUG
    printk("check_otp_flag:reg 0x3404flag=%d",flag);
#endif


   if (customer_id==custom_TRULY)
	flag>>=6;

   
    if (!flag) {
        return 0;
    }
    else if(flag == 0x01) {
        return 2;
    }
    else {
        return 1;
    }
}

// index: index of otp group. (0, 1, 2)
// return: 0, group index is empty
//         1, group index has invalid data
//         2, group index has valid data
/*static int check_otp_lenc(uint index)
{
    uint flag, page, temp;
    uint check_time =20; 

    // select page
    page = -3 + index*6;
    IMX178MIPI_write_cmos_sensor(0x3402, page);
    // turn on otp read mode
    IMX178MIPI_write_cmos_sensor(0x3400, 0x01);
    // Status check : 0x3B01 = 0x01 (bit0 1:read ready)
    temp = IMX178MIPI_read_cmos_sensor(0x3401);
   while(check_time--)
   {
	    temp = IMX178MIPI_read_cmos_sensor(0x3401);
	    if ((temp&0x01) ==0x1)
	    {
	     #ifdef IMX178_OTP_DEBUG
	        printk("truly] check_otp_awb:Status check : 0x3401 == 0x01, read  ready (bit0 1:read  ready)\n");
	      #endif
		  break;
	    }	
   }

   if(check_time == 0)
   	 printk("truly] check_otp_awb:Status check wait:  check_time=%d",check_time);
    // read flag
    flag = IMX178MIPI_read_cmos_sensor(0x3404);
    if (!flag) {
        return 0;
    }
    else if (flag == 0x01) {
        return 2;
    }
    else {
        return 1;
    }
}*/

static int read_otp(uint page, uint32_t address, uint* array, uint32_t size)
{
    uint32_t i = 0, j = 0;
    uint temp;
    kal_bool res = KAL_TRUE;
    uint check_time =10;


    while (j<size) {
        i = 0;
        IMX178MIPI_write_cmos_sensor(0x3402, page);
        // turn on otp read mode
        IMX178MIPI_write_cmos_sensor(0x3400, 0x01);
        mdelay(100);
        // Status check : 0x3B01 = 0x01 (bit0 1:read ready)
        temp = IMX178MIPI_read_cmos_sensor(0x3401);
        while(check_time--)
        {
	     	    temp = IMX178MIPI_read_cmos_sensor(0x3401);
	     	    if ((temp&0x01) ==0x0)
	     	    {
	     	     #ifdef IMX178_OTP_DEBUG
	     	        printk(" check_otp_awb:Status check : 0x3401 != 0x01, read  ready (bit0 0:read  ready)\n");
	     	      #endif
	     		  break;
	     	    }	
        }

   	if(check_time == 0)
   	   printk("check_otp_awb:Status check wait:  check_time=%d",check_time);

        while(i<64)
        {
            *(array+j) = IMX178MIPI_read_cmos_sensor(address+i);
            #ifdef IMX178_OTP_DEBUG
            printk(" read_otp [%d] == 0x%x\n",j,array[j]);
            #endif
            i++;
            j++;
            if (j>=size)
            {
                break;
            }
        }
        page++;
        #ifdef IMX178_OTP_DEBUG
	printk(" page = 0x%x\n",page);
        #endif
    }
    return 0;
}

// R/G and B/G of typical camera module is defined here
//add for distinguish diffrent moudle Truly or Liteon
static uint32_t RG_Ratio_typical=0x00;
static uint32_t BG_Ratio_typical=0x00;
static uint32_t GG_Ratio_typical=0x00;

static uint32_t RG_Ratio_typical_Truly=0x022d;
static uint32_t BG_Ratio_typical_Truly=0x0213;
static uint32_t GG_Ratio_typical_Truly=0x0400;

static uint32_t RG_Ratio_typical_Sunny=0x0246;
static uint32_t BG_Ratio_typical_Sunny=0x0237;
static uint32_t GG_Ratio_typical_Sunny=0x0400;

// index: index of otp group. (0, 1, 2)
// return:0,
static int read_otp_wb(uint index, struct otp_struct* otp)
{
    uint32_t address=0x3404, page, temp;
    uint arr[42]={0};

    //1 select page
    page = 0x00 + index;    
/*
    IMX178MIPI_write_cmos_sensor(0x3B02, page);
    // turn on otp read mode
    IMX178MIPI_write_cmos_sensor(0x3B00, 0x01);
    mdelay(100);
    // Status check : 0x3B01 = 0x01 (bit0 1:read ready)
    temp = IMX178MIPI_read_cmos_sensor(0x3B01);
    if ((temp&0x01) ==0x01)
    {
        printk("[Sunny] read_otp_wb:Status check : 0x3B01 == 0x01, read ready (bit0 1:read ready)\n");
    }
    else
    {
        printk("[Sunny] read_otp_wb:Status check : 0x3B01 != 0x01, read not ready (bit0 1:read ready)\n");
        return 1;
    }

    otp->year = IMX178MIPI_read_cmos_sensor(address+3);
    otp->month = IMX178MIPI_read_cmos_sensor(address+4);
    otp->day = IMX178MIPI_read_cmos_sensor(address+5);
    otp->customer_id = IMX178MIPI_read_cmos_sensor(address+6);
    otp->lens_id = IMX178MIPI_read_cmos_sensor(address+7);
    otp->vcm_id = IMX178MIPI_read_cmos_sensor(address+8);
    otp->rg_ratio = (IMX178MIPI_read_cmos_sensor(address+15)<<8)|IMX178MIPI_read_cmos_sensor(address+16);
    otp->bg_ratio = (IMX178MIPI_read_cmos_sensor(address+17)<<8)|IMX178MIPI_read_cmos_sensor(address+18);
    otp->gg_ratio = (IMX178MIPI_read_cmos_sensor(address+19)<<8)|IMX178MIPI_read_cmos_sensor(address+20);
*/
    //2 read otp awb gain data
    read_otp(page, address, arr, 42);

    #ifdef IMX178_OTP_DEBUG
    printk("read_otp_wb:arr[15] == 0x%x\n",arr[15]);
    printk("read_otp_wb:arr[16] == 0x%x\n",arr[16]);
    printk("read_otp_wb:arr[17] == 0x%x\n",arr[17]);

    printk("read_otp_wb:arr[18] == 0x%x\n",arr[18]);
    printk("read_otp_wb:arr[19] == 0x%x\n",arr[19]);
    printk("read_otp_wb:arr[20] == 0x%x\n",arr[20]);
    #endif
	
   if (customer_id==custom_TRULY){
   	    uint current_g;
	    otp->customer_id = arr[1];
	    otp->year = arr[2];
	    otp->month = arr[3];
	    otp->day = arr[4];

	    otp->lens_id = arr[5];
	    otp->vcm_id = arr[6];

	    current_g= ((arr[14]<<8)|arr[15] + (arr[16]<<8)|arr[17])/2;
		 	
	    otp->rg_ratio =1024* ( (arr[12]<<8)|arr[13]) /current_g;
	    otp->bg_ratio = 1024 * ((arr[18]<<8)|arr[19]) /current_g;

	     current_g= ((arr[22]<<8)|arr[23] + (arr[24]<<8)|arr[25])/2;
		 	
	    RG_Ratio_typical_Truly =1024* ( (arr[20]<<8)|arr[21]) /current_g;
	    BG_Ratio_typical_Truly = 1024 * ((arr[26]<<8)|arr[27]) /current_g;
		
   }
   else{
   	
	    //3 check sum
	    uint sum=0, i;
	    for(i=2;i<42;i++)
	    {
	        sum = sum + arr[i];
	    }
	    #ifdef IMX135_OTP_DEBUG
	    printk("[Sunny] read_otp_wb:arr[1] == 0x%x\n",arr[1]);
	    printk("[Sunny] read_otp_wb:sum == 0x%x\n",sum%256);
	    #endif
	    if(arr[1] != (sum%256))
	    {
		printk("[Sunny] read_otp_wb:read otp data error!!!\n");
		return 1;
	    }

	    otp->year = arr[3];
	    otp->month = arr[4];
	    otp->day = arr[5];
	    otp->customer_id = arr[6];
	    otp->lens_id = arr[7];
	    otp->vcm_id = arr[8];
	    otp->rg_ratio = (arr[15]<<8)|arr[16];
	    otp->bg_ratio = (arr[17]<<8)|arr[18];
	    otp->gg_ratio = (arr[19]<<8)|arr[20];
	
   }

    return 0;
}

#if 0
// index: index of otp group. (0, 1, 2)
// return:0
static int read_otp_lenc(uint index, struct otp_struct* otp)
{
    uint page, i, j=0, k = 3, temp;
    uint32_t address=0x3B04;
    uint arr[384]={0};

    //1 select page 3/9
    page = -3 + index*6;
/*
    while (j<384) {
        i = 0;
        IMX178MIPI_write_cmos_sensor(0x3B02, page);
        // turn on otp read mode
        IMX178MIPI_write_cmos_sensor(0x3B00, 0x01);
        mdelay(100);
        // Status check : 0x3B01 = 0x01 (bit0 1:read ready)
        temp = IMX178MIPI_read_cmos_sensor(0x3B01);
        if ((temp&0x01) ==0x01)
        {
            printk("[Sunny] read_otp_lenc:Status check : 0x3B01 == 0x01, read ready (bit0 1:read ready)\n");
        }
        else
        {
            printk("[Sunny] read_otp_lenc:Status check : 0x3B01 != 0x01, read not ready (bit0 1:read ready)\n");
            return 1;
        }
        while(i<64)
        {
            address = 0x3B04;
            arr[j] = IMX178MIPI_read_cmos_sensor(address+i);
            printk("[Sunny] read_otp_lenc shading[%d] == 0x%x\n",j,arr[j]);
            i++;
            j++;
            if (j>=384)
            {
                break;
            }
        }
        page++;
    }
*/
    //2 read otp lsc data
    read_otp(page, address, arr, 384);

    //3 check sum
    uint sum=0;
    for (i=0; i<504; i+=4) {
        otp->lenc[i+0] = arr[k]>>4;
        sum = sum + arr[k];
        otp->lenc[i+1] = arr[k+1];
        sum = sum + arr[k+1];
        otp->lenc[i+2] = arr[k]&0x0f;
        sum = sum + arr[k+2];
        otp->lenc[i+3] = arr[k+2];
	k += 3;
    }
    sum = sum + arr[2];
    #ifdef IMX178_OTP_DEBUG
    printk("[Sunny] read_otp_lenc:arr[1] == 0x%x\n",arr[1]);
    printk("[Sunny] read_otp_lenc:sum == 0x%x\n",sum%256);
    #endif
    if(arr[1] != (sum%256))
    {
	printk("[Sunny] read_otp_lenc:read otp data error!!!\n");
	return 1;
    }

    #ifdef IMX178_OTP_DEBUG
    for (i=0; i<504; i++) {
        printk("[Sunny] read_otp_lenc lsc[%d] == 0x%x\n",i,otp->lenc[i]);
    }
    #endif

    return 0;
}
#endif
// R_gain: red gain of sensor AWB, 0x400 = 1
// G_gain: green gain of sensor AWB, 0x400 = 1
// B_gain: blue gain of sensor AWB, 0x400 = 1
// return 0
int imx178_update_awb_gain()//uint32_t R_gain, uint32_t G_gain, uint32_t B_gain)
{
    #ifdef IMX178_OTP_DEBUG
    printk("[Sunny] update_awb_gain() call.\n");
    #endif
    if(0 == wb_flag)
    {
	#ifdef IMX178_OTP_DEBUG
	printk("[Sunny] update_awb_gain() call, wb_flag == 0.\n");
	#endif
        //reset the digital gain
        #if 0
        IMX135MIPI_write_cmos_sensor(0x020F, G_gain1 & 0xFF);
        IMX135MIPI_write_cmos_sensor(0x0211, R_gain1 & 0xFF);
        IMX135MIPI_write_cmos_sensor(0x0213, B_gain1 & 0xFF);
        IMX135MIPI_write_cmos_sensor(0x0215, G_gain1 & 0xFF);
	 #endif
	IMX178MIPI_write_cmos_sensor(0x020E, G_gain1_R>>8);
	IMX178MIPI_write_cmos_sensor(0x020F, G_gain1_R& 0xFF);
	IMX178MIPI_write_cmos_sensor(0x0210, R_gain1 >>8);
	IMX178MIPI_write_cmos_sensor(0x0211, R_gain1 & 0xFF);
	IMX178MIPI_write_cmos_sensor(0x0212, B_gain1 >>8);
	IMX178MIPI_write_cmos_sensor(0x0213, B_gain1 & 0xFF);
	IMX178MIPI_write_cmos_sensor(0x0214, G_gain1_B>>8);
	IMX178MIPI_write_cmos_sensor(0x0215, G_gain1_B& 0xFF);
        return 0;
    }
}

/*
int imx178_update_lens()//(struct otp_struct otp)
{
    printk("[Sunny] update_lens() call ++.\n");
    uint i;

    if(0 == lenc_flag)
    {
	#ifdef IMX178_OTP_DEBUG
	printk("[Sunny] update_lens() call, lenc_flag == 0.\n");
	#endif
        //Turn off lsc
        IMX178MIPI_write_cmos_sensor(0x4500, 0x00);
        IMX178MIPI_write_cmos_sensor(0x0700, 0x00);
        IMX178MIPI_write_cmos_sensor(0x3A63, 0x00);

        //Access LSC table //table1
        for(i=0;i<504;i++) {
            IMX178MIPI_write_cmos_sensor(0x4800+i, current_otp2.lenc[i]);
            #ifdef IMX178_OTP_DEBUG
            printk("[mingji] current_otp2.lenc[%d] == 0x%x\n",i,current_otp2.lenc[i]);
            #endif
        }

        //Turn on lsc
        IMX178MIPI_write_cmos_sensor(0x4500, 0x1f);
        IMX178MIPI_write_cmos_sensor(0x0700, 0x01);
        IMX178MIPI_write_cmos_sensor(0x3A63, 0x01);

        return 0;
    }
    printk("[Sunny] update_lens() call --.\n");
}*/




// call this function after ov8825 initialization
// return value: 0, update success
//               1, no OTP
int imx178_update_otp_wb(void)
{
    int i, temp, otp_index;
//    struct otp_struct current_otp;
    uint32_t R_gain, G_gain, B_gain, G_gain_R, G_gain_B;
	
    // R/G and B/G of current camera module is read out from sensor OTP
    // check first wb OTP with valid data
    printk(" update_otp_wb++\n");
    for(i=3;i>0;i--) {
        temp = check_otp_awb(i);
        if (temp == 2) {
            otp_index = i;
            break;
        }
    }
    if(i==0) {
        // no valid wb OTP data
        printk(" no valid wb OTP data\n");
        return 1;
    }

    memset(&current_otp, 0, sizeof(current_otp));
    wb_flag = read_otp_wb(otp_index, &current_otp);
    if(0 == wb_flag)
    {
        //add for distinguish diffrent moudle Truly or Liteon
        printk(" #########current_otp->customer_id = 0x%x\n",current_otp.customer_id);

 //       if(current_otp.customer_id == 0x01)
	 if(customer_id == 0x01)
        {
            RG_Ratio_typical = RG_Ratio_typical_Sunny;
            BG_Ratio_typical = BG_Ratio_typical_Sunny;
            GG_Ratio_typical = GG_Ratio_typical_Sunny;
        }
//        else if(current_otp.customer_id == 0x02)
	 else if(customer_id == 0x02)		
        {
            RG_Ratio_typical = RG_Ratio_typical_Truly;
            BG_Ratio_typical = BG_Ratio_typical_Truly;
            GG_Ratio_typical = GG_Ratio_typical_Truly;
        }

        //calculate gain
        //0x400 = 1x gain
        printk(" #########current_otp.rg_ratio = 0x%x\n",current_otp.rg_ratio);
        printk(" #########current_otp.bg_ratio = 0x%x\n",current_otp.bg_ratio);
        printk(" #########current_otp.gg_ratio = 0x%x\n",current_otp.gg_ratio);
        printk(" #########RG_Ratio_typical = 0x%x\n",RG_Ratio_typical);
        printk(" #########BG_Ratio_typical = 0x%x\n",BG_Ratio_typical);
        printk(" #########GG_Ratio_typical = 0x%x\n",GG_Ratio_typical);

#if 0
        if(current_otp.bg_ratio < BG_Ratio_typical)
        {
            if (current_otp.rg_ratio < RG_Ratio_typical)
            {
                G_gain = 0x100;
                B_gain = 0x100 * BG_Ratio_typical / current_otp.bg_ratio;
                R_gain = 0x100 * RG_Ratio_typical / current_otp.rg_ratio;
            }
            else
            {
                R_gain = 0x100;
                G_gain = 0x100 * current_otp.rg_ratio / RG_Ratio_typical;
                B_gain = G_gain * BG_Ratio_typical / current_otp.bg_ratio;
            }
        }
        else
        {
            if (current_otp.rg_ratio < RG_Ratio_typical)
            {
                B_gain = 0x100;
                G_gain = 0x100 * current_otp.bg_ratio / BG_Ratio_typical;
                R_gain = G_gain * RG_Ratio_typical / current_otp.rg_ratio;
            }
            else
            {
                G_gain_B = 0x100 * current_otp.bg_ratio / BG_Ratio_typical;
                G_gain_R = 0x100 * current_otp.rg_ratio / RG_Ratio_typical;
                if(G_gain_B > G_gain_R )
                {
                    B_gain = 0x100;
                    G_gain = G_gain_B;
                    R_gain = G_gain * RG_Ratio_typical / current_otp.rg_ratio;
                }
                else
                {
                    R_gain = 0x100;
                    G_gain = G_gain_R;
                    B_gain = G_gain * BG_Ratio_typical / current_otp.bg_ratio;
                }
            }
        }
        // write sensor wb gain to registers
        R_gain1=R_gain;
        G_gain1=G_gain;
        B_gain1=B_gain;
        #ifdef IMX178_OTP_DEBUG
        printk(" R_gain1 == 0x%x\n",R_gain1);
        printk(" G_gain1 == 0x%x\n",G_gain1);
        printk(" B_gain1 == 0x%x\n",B_gain1);

        printk(" R_gain1&0xFF == 0x%x\n",R_gain1&0xFF);
        printk(" G_gain1&0xFF == 0x%x\n",G_gain1&0xFF);
        printk(" B_gain1&0xFF == 0x%x\n",B_gain1&0xFF);
        #endif
#endif

	R_gain = 512 * RG_Ratio_typical / current_otp.rg_ratio;
	B_gain = 512 * BG_Ratio_typical / current_otp.bg_ratio;

	if(R_gain >= 512 && B_gain >= 512)
	{
		R_gain1 = (USHORT)(GAIN_DEFAULT * R_gain / 512);
		G_gain1_R = GAIN_DEFAULT; //0x0100
		G_gain1_B = GAIN_DEFAULT;     //0x0100        
		B_gain1 = (USHORT)(GAIN_DEFAULT * B_gain / 512);
	}
	else if(R_gain >= B_gain && B_gain < 512)
	{
		R_gain1 =  (USHORT)(GAIN_DEFAULT * R_gain / B_gain );
		G_gain1_R = (USHORT)(GAIN_DEFAULT * 512 / B_gain );
		G_gain1_B = (USHORT)(GAIN_DEFAULT * 512 / B_gain );     
		B_gain1 = GAIN_DEFAULT;    //0x0100 
	} 
	else
	{
		R_gain1 = GAIN_DEFAULT;    //0x0100 
		G_gain1_R = (USHORT)(GAIN_DEFAULT * 512 / R_gain );
		G_gain1_B = (USHORT)(GAIN_DEFAULT * 512 / R_gain );      
		B_gain1 =  (USHORT)(GAIN_DEFAULT * B_gain / R_gain );
	} 
	 #ifdef IMX178_OTP_DEBUG
        printk(" R_gain1 == 0x%x\n",R_gain1);
        printk(" G_gain1 == 0x%x\n",G_gain1);
        printk(" B_gain1 == 0x%x\n",B_gain1);
	 printk(" G_gain1_R == 0x%x\n",G_gain1_R);
	 printk(" G_gain1_B == 0x%x\n",G_gain1_B);
        #endif
		
    }

    printk("update_otp_wb--\n");
    return 0;
}

// call this function after ov8825 initialization
// return value: 0 update success
//               1, no OTP
/*int imx178_update_otp_lenc(void)
{
    uint i, temp, otp_index;
    struct otp_struct current_otp;
    printk("[Sunny] update_otp_lenc++\n");
    // check first lens correction OTP with valid data
    for(i=2;i>0;i--) {
        temp = check_otp_lenc(i);
        if (temp == 2) {
            otp_index = i;
            break;
        }
    }
    if (i==0) {
        // no lens correction data
        printk("[Sunny] no lens correction data\n");
        return 1;
    }
    lenc_flag = read_otp_lenc(otp_index, &current_otp);
    current_otp2=current_otp;

    //success
    printk("[Sunny] update_otp_lenc--\n");
    return 0;
}*/


//extern int IMM_GetOneChannelValue_Cali(int Channel, int*voltage);
int get_sensor_imx178_customer_id()
{
    int i, temp, otp_index;
//    struct otp_struct current_otp;

 /*   int camera_id_volt = 0;
    int ret = 0;

   ret = IMM_GetOneChannelValue_Cali(0, &camera_id_volt);
   if(ret != 0)
	    printk("[get_sensor_imx178_customer_id]camera_id_volt read fail\n");
    else
	    printk("[get_sensor_imx178_customer_id]camera_id_volt = %d\n", camera_id_volt);

   camera_id_volt/=1000;          //volt uV to mV
    if (( 600 < camera_id_volt )&&(camera_id_volt< 1200) )			//truly ID volt is 0.9V. shunyu is 0V
    {
	customer_id=2;
    }
    else if(camera_id_volt< 300)
    {
	customer_id=1;
    }
    else
    {
	customer_id=0;
    }*/

/*   mt_set_gpio_mode(GPIO82|0x80000000,GPIO_MODE_00);	
   mt_set_gpio_dir(GPIO82|0x80000000,GPIO_DIR_IN);
   if(GPIO_OUT_ONE == mt_get_gpio_in(GPIO82|0x80000000))
   	customer_id=2;	
   else		
   	customer_id=1;

    printk("[get_sensor_imx178_customer_id =%d\n",customer_id);*/
    IMX178MIPI_write_cmos_sensor(0x0100, 0x0);
		
    for(i=3;i>0;i--) {
        temp = check_otp_awb(i);
        if (temp == 2) {
            otp_index = i;
            break;
        }
    }
    printk("otp_index=%d,i=%d\n",otp_index,i);
    if(i == 0) {
        printk("no valid wb OTP data\n");
        return 0;
    }
    memset(&current_otp, 0, sizeof(current_otp));
    if(0 == read_otp_wb(otp_index, &current_otp))
    {
	customer_id = current_otp.customer_id;
 	printk("current_otp.customer_id =%d\n",current_otp.customer_id);
    }

     return customer_id;
}

