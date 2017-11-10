/*
 *
 * FocalTech TouchScreen driver.
 * 
 * Copyright (c) 2010-2015, Focaltech Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

 /*******************************************************************************
*
* File Name: Focaltech_Gestrue.c
*
* Author: Xu YongFeng
*
* Created: 2015-01-29
*   
* Modify by mshl on 2015-03-20
*
* Abstract:
*
* Reference:
*
*******************************************************************************/

/*******************************************************************************
* 1.Included header files
*******************************************************************************/
#include "focaltech_core.h"
//int gesture_onoff_fts = 0;
#if FTS_GESTRUE_EN
#include "ft_gesture_lib.h"
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include<linux/wakelock.h>
#define  TPD_PACKET_LENGTH                     128
#define PROC_READ_DRAWDATA			10

int gesture_onoff_fts = 0;
/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
#define GESTURE_LEFT								    0x20
#define GESTURE_RIGHT								    0x21
#define GESTURE_UP		    							0x22
#define GESTURE_DOWN							     	0x23
#define GESTURE_DOUBLECLICK						        0x24
#define GESTURE_O		    							0x30
#define GESTURE_W		    							0x31
#define GESTURE_M		   	 						    0x32
#define GESTURE_E		    							0x33
#define GESTURE_L		    							0x44
#define GESTURE_S		    							0x46
#define GESTURE_V		    							0x54
#define GESTURE_Z		    							0x65
#define GESTURE_C		    					        0x34

#define KEY_GESTURE		                                195 //zhangwenkang.wt 20161011 open
#define DOUBLE_TAP                                      0xA0  

#define FTS_GESTRUE_POINTS 						        255
#define FTS_GESTRUE_POINTS_ONETIME  			    	62
#define FTS_GESTRUE_POINTS_HEADER 				        8
#define FTS_GESTURE_OUTPUT_ADRESS 				        0xD3
#define FTS_GESTURE_OUTPUT_UNIT_LENGTH 			        4
#define PROC_UPGRADE							        0
#define TS_WAKE_LOCK_TIMEOUT		(2 * HZ)

unsigned short gesture_id;
	
		extern u8 fts_gesture_three_byte_one;
		extern u8 fts_gesture_three_byte_two;
		extern u8 fts_gesture_three_byte_three;
		extern u8 fts_gesture_three_byte_four;
		extern int fts_gesture_data;
		extern int fts_gesture_ctrl;
		extern int fts_gesture_mask;
		//static int gesture_data1=0;
		//static int gesture_data2=0;
		//static int gesture_data3=0;


extern struct wake_lock gesture_chrg_lock;

//static unsigned char proc_operate_mode = PROC_UPGRADE;

unsigned char buf[FTS_GESTRUE_POINTS * 2] ;

/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/


/*******************************************************************************
* Static variables
*******************************************************************************/
short pointnum 					= 0;
unsigned short coordinate_x[150] 	= {0};
unsigned short coordinate_y[150] 	= {0};

/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/

/*******************************************************************************
* Static function prototypes
*******************************************************************************/


/*******************************************************************************
*   Name: fts_Gesture_init
*  Brief:
*  Input:
* Output: None
* Return: None
*******************************************************************************/


int fts_Gesture_init(struct input_dev *input_dev)
{
        //init_para(480,854,60,0,0);

	input_set_capability(input_dev, EV_KEY, KEY_POWER);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_U); 
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_UP); 
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_DOWN);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_LEFT); 
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_RIGHT); 
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_O);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_E); 
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_M); 
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_L);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_W);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_S); 
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_V);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_Z);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_C);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE);
	
	__set_bit(KEY_GESTURE_RIGHT, input_dev->keybit);
	__set_bit(KEY_GESTURE_LEFT, input_dev->keybit);
	__set_bit(KEY_GESTURE_UP, input_dev->keybit);
	__set_bit(KEY_GESTURE_DOWN, input_dev->keybit);
	__set_bit(KEY_GESTURE_U, input_dev->keybit);
	__set_bit(KEY_GESTURE_O, input_dev->keybit);
	__set_bit(KEY_GESTURE_E, input_dev->keybit);
	__set_bit(KEY_GESTURE_M, input_dev->keybit);
	__set_bit(KEY_GESTURE_W, input_dev->keybit);
	__set_bit(KEY_GESTURE_L, input_dev->keybit);
	__set_bit(KEY_GESTURE_S, input_dev->keybit);
	__set_bit(KEY_GESTURE_V, input_dev->keybit);
	__set_bit(KEY_GESTURE_Z, input_dev->keybit);
	__set_bit(KEY_GESTURE_C, input_dev->keybit);
	__set_bit(KEY_GESTURE, input_dev->keybit);
	return 0;
}

/*******************************************************************************
*   Name: fts_check_gesture
*  Brief:
*  Input:
* Output: None
* Return: None
*******************************************************************************/
static void fts_check_gesture(int gesture_id)
{

	printk("[FTS]fts_check_gesture start gesture_id==0x%x\n ",gesture_id);
    wake_lock_timeout(&gesture_chrg_lock,TS_WAKE_LOCK_TIMEOUT);
if(gesture_onoff_fts==0)
{
   gesture_id = 0x00 ;
  	printk("[FTS]gesture_id==0x%x\n ",gesture_id);
	return;
}
		   if ((gesture_id == GESTURE_C) || (gesture_id == GESTURE_E) || (gesture_id == GESTURE_M) || 
			   (gesture_id ==GESTURE_O) || (gesture_id == GESTURE_S) || (gesture_id == GESTURE_V) || 
			   (gesture_id == GESTURE_W) || (gesture_id == GESTURE_Z) 
			   )
		   {
			   if ((((gesture_id == GESTURE_V) && ((fts_gesture_three_byte_three & 0x01) == 0x01)) || 
			((gesture_id == GESTURE_C) && ((fts_gesture_three_byte_three & 0x02) == 0x02)) || 
			((gesture_id == GESTURE_E) && ((fts_gesture_three_byte_three & 0x04) == 0x04)) || 
			((gesture_id == GESTURE_W) && ((fts_gesture_three_byte_three & 0x08) == 0x08)) || 
			((gesture_id == GESTURE_M) && ((fts_gesture_three_byte_three & 0x10) == 0x10)) || 
			((gesture_id == GESTURE_S) && ((fts_gesture_three_byte_three & 0x20) == 0x20)) || 
			((gesture_id == GESTURE_Z) && ((fts_gesture_three_byte_three & 0x40) == 0x40)) ||
			((gesture_id == GESTURE_O) && ((fts_gesture_three_byte_three & 0x80) == 0x80)) 
			   )&&(fts_gesture_mask !=0))
			   {
			   printk("[FTS]:Wakeup by gesture(%c), light up the screen!", gesture_id);
				   input_report_key(tpd->dev, KEY_GESTURE, 1);
				   input_sync(tpd->dev);
				   input_report_key(tpd->dev, KEY_GESTURE, 0);
				   input_sync(tpd->dev);

		   if(gesture_id == GESTURE_V)
		   {
			   fts_gesture_data = 0xC7;
		   }
		   else if(gesture_id == GESTURE_C)
		   {
			   fts_gesture_data = 0xC1;
		   }
		   else if(gesture_id == GESTURE_E)
		   {
			   fts_gesture_data = 0xC0;
		   }
		   else if(gesture_id == GESTURE_W)
		   {
			   fts_gesture_data = 0xC2;
		   }
		   else if(gesture_id == GESTURE_M)
		   {
			   fts_gesture_data = 0xC3;
		   }
		   else if(gesture_id == GESTURE_S)
		   {
			   fts_gesture_data = 0xC5;
		   }
		   else if(gesture_id == GESTURE_Z)
		   {
			   fts_gesture_data = 0xCA;
		   }
		   else  //gesture_id == 'o'
		   {
			   fts_gesture_data = 0xC4;
		   }
				   // clear 0x814B
				   gesture_id = 0x00;
			   }
	   else
	   {
				   gesture_id = 0x00;
	   }
		   }
		   else if ( (gesture_id == 0x20) || (gesture_id == 0x21) ||(gesture_id == 0x22) || (gesture_id == 0x23) )
		   {
	   if((((gesture_id == 0x21) && ((fts_gesture_three_byte_four & 0x01) == 0x01)) ||
		   ((gesture_id == 0x23) && ((fts_gesture_three_byte_four & 0x04) == 0x04)) ||
		   ((gesture_id == 0x22) && ((fts_gesture_three_byte_four & 0x08) == 0x08)) ||
		   ((gesture_id == 0x20) && ((fts_gesture_three_byte_four & 0x02) ==0x02))
	   )&&(fts_gesture_mask !=0))
	   {
				   printk("[FTS]: %x slide to light up the screen!", gesture_id);

				   input_report_key(tpd->dev, KEY_GESTURE, 1);
				   input_sync(tpd->dev);
				   input_report_key(tpd->dev, KEY_GESTURE, 0);
				   input_sync(tpd->dev);

		   if(gesture_id == 0x21)
		   {
				fts_gesture_data = 0xB1;
		   }
		   else if(gesture_id == 0x23)
		   {
			   fts_gesture_data = 0xB3;
		   }
		   else if(gesture_id == 0x22)
		   {
			   fts_gesture_data = 0xB2; 
		   }
		   else   //type==3
		   {
			   fts_gesture_data = 0xB0;
		   }

	   }
	   else
	   {
		   // clear 0x814B
				   gesture_id = 0x00;
	   }
		   }
		   else if (0x24 == gesture_id)
		   {
		   
		   printk("meizu_test_zx:Double click to light up the screen!");
			   if(((fts_gesture_three_byte_two & 0x01)== 0x01)&&(fts_gesture_mask !=0))
			   {
					printk("[FTS]:Double click to light up the screen!");

				input_report_key(tpd->dev, KEY_GESTURE, 1);
					input_sync(tpd->dev);
					input_report_key(tpd->dev, KEY_GESTURE, 0);
					input_sync(tpd->dev);
					// clear 0x814B
					gesture_id = 0x00;
					
		   fts_gesture_data = DOUBLE_TAP;
			   }
	   else
	   {
				   gesture_id = 0x00;
	   }
		   }
		   else
		   {
			   gesture_id = 0x00;
		   }

}

 /************************************************************************
*   Name: fts_read_Gestruedata
* Brief: read data from TP register
* Input: no
* Output: no
* Return: fail <0
***********************************************************************/

int fts_read_Gestruedata(void)
{   
		unsigned char buf[FTS_GESTRUE_POINTS * 4+2+6] = { 0 };
		//unsigned char buf[FTS_GESTRUE_POINTS * 2] = { 0 }; 
		int ret = -1;
		int gestrue_id = 0;
		short pointnum = 0;
		buf[0] = 0xd3;
		
		ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, FTS_GESTRUE_POINTS_HEADER);
		if (ret < 0)
		{
			printk( "%s [FTS]read touchdata failed.\n", __func__);
			return ret;
		}
		//printk("ft5x0x_read_Touchdata buf[0]=%x \n",buf[0]);
		/* FW */

  		if(buf[0]!=0xfe){
      			  gestrue_id =  buf[0];
      			  //check_gesture(gestrue_id);
		}
		
    		pointnum = (short)(buf[1]) & 0xff;
			printk("[FTS] the pointnum = %d\n",pointnum);			
    		buf[0] = 0xd3;
    		if((pointnum * 4 + 2 +36)<255) //  + 6
    		{
    			ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, (pointnum * 4 + 2 + 36)); //  + 6
   		 	}else{
    			 ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, 255);
         		 ret = fts_i2c_read(fts_i2c_client, buf, 0, buf+255, (pointnum * 4 + 2 + 36)-255); // + 6
    		}
			
   			if (ret < 0)
    		{
        		printk( "%s [FTS]read touchdata failed.\n", __func__);
        		return ret;
    		}

			fts_check_gesture(gestrue_id);
        	 return 0;
 }
	




#endif
