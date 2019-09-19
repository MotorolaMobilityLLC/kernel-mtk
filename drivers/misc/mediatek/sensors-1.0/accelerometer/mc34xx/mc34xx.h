/*****************************************************************************
 *
 * Copyright (c) 2014 mCube, Inc.  All rights reserved.
 *
 * This source is subject to the mCube Software License.
 * This software is protected by Copyright and the information and source code
 * contained herein is confidential. The software including the source code
 * may not be copied and the information contained herein may not be used or
 * disclosed except with the written permission of mCube Inc.
 *
 * All other rights reserved.
 *
 * This code and information are provided "as is" without warranty of any
 * kind, either expressed or implied, including but not limited to the
 * implied warranties of merchantability and/or fitness for a
 * particular purpose.
 *
 * The following software/firmware and/or related documentation ("mCube Software")
 * have been modified by mCube Inc. All revisions are subject to any receiver's
 * applicable license agreements with mCube Inc.
 *
 * Accelerometer Sensor Driver
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
 *****************************************************************************/

#ifndef _MC34XX_H_
	#define _MC34XX_H_

#include <linux/ioctl.h>

/***********************************************
 *** REGISTER MAP
 ***********************************************/
#define MC34XX_REG_XOUT                    			0x00
#define MC34XX_REG_YOUT                    			0x01
#define MC34XX_REG_ZOUT                    			0x02

#define MC34XX_REG_INTERRUPT_ENABLE    				0x06
#define MC34XX_REG_MODE_FEATURE            			0x07
#define MC34XX_REG_SAMPLE_RATE             			0x08

#define MC34XX_REG_XOUT_EX_L               			0x0D
#define MC34XX_REG_XOUT_EX_H		            	0x0E
#define MC34XX_REG_YOUT_EX_L        		    	0x0F
#define MC34XX_REG_YOUT_EX_H               			0x10
#define MC34XX_REG_ZOUT_EX_L		            	0x11
#define MC34XX_REG_ZOUT_EX_H        		    	0x12

#define MC34XX_REG_CHIPID							0x18
#define MC34XX_REG_LPF_RANGE_RES          			0x20
#define MC34XX_REG_MCLK_POLARITY          			0x2A
#define MC34XX_REG_PRODUCT_CODE_H      				0x34
#define MC34XX_REG_PRODUCT_CODE_L      				0x3B

#ifndef MERAK
    #define MERAK		//MENSA = MC3413, 3433
    
#define MC34X3_REG_TILT_STATUS         				0x03
#define MC34X3_REG_SAMPLE_RATE_STATUS  				0x04
#define MC34X3_REG_SLEEP_COUNT         				0x05
                                          		
#define MC34X3_REG_TAP_DETECTION_ENABLE   			0x09
#define MC34X3_REG_TAP_DWELL_REJECT    				0x0A
#define MC34X3_REG_DROP_CONTROL        				0x0B
#define MC34X3_REG_SHAKE_DEBOUNCE      				0x0C
                                          		
#define MC34X3_REG_SDM_X							0x14
#define MC34X3_REG_MISC								0x17                                         	

#define MC34X3_REG_SHAKE_THRESHOLD					0x2B
#define MC34X3_REG_UD_Z_TH             				0x2C
#define MC34X3_REG_UD_X_TH             				0x2D
#define MC34X3_REG_RL_Z_TH             				0x2E
#define MC34X3_REG_RL_Y_TH             				0x2F
#define MC34X3_REG_FB_Z_TH             				0x30
#define MC34X3_REG_DROP_THRESHOLD      				0x31
#define MC34X3_REG_TAP_THRESHOLD       				0x32
#define MC34X3_REG_PRODUCT_CODE        				0x3B                                          
#endif

#ifndef MENSA    
    #define MENSA		//MENSA = MC3416, 3436
    
#define MC34X6_REG_STATUS_1 						0x03
#define MC34X6_REG_INTR_STAT_1						0x04
#define MC34X6_REG_DEVICE_STATUS					0x05
                                          		
#define MC34X6_REG_MOTION_CTRL		   				0x09	
#define MC34X6_REG_STATUS_2 						0x13
#define MC34X6_REG_INTR_STAT_2						0x14
#define MC34X6_REG_SDM_X							0x15
                                          		
#define MC34X6_REG_RANGE_CONTROL          			0x20
#define MC34X6_REG_MISC 							0x2C	
                                          		
#define MC34X6_REG_TF_THRESHOLD_LSB 				0x40
#define MC34X6_REG_TF_THRESHOLD_MSB 				0x41
#define MC34X6_REG_TF_DB							0x42
#define MC34X6_REG_AM_THRESHOLD_LSB 				0x43
#define MC34X6_REG_AM_THRESHOLD_MSB 				0x44
#define MC34X6_REG_AM_DB							0x45
#define MC34X6_REG_SHK_THRESHOLD_LSB				0x46
#define MC34X6_REG_SHK_THRESHOLD_MSB				0x47
#define MC34X6_REG_PK_P2P_DUR_THRESHOLD_LSB 		0x48
#define MC34X6_REG_PK_P2P_DUR_THRESHOLD_MSB 		0x49
#define MC34X6_REG_TIMER_CTRL						0x4A
#endif

/***********************************************
 *** RETURN CODE
 ***********************************************/
#define MC34XX_RETCODE_SUCCESS                 (0)
#define MC34XX_RETCODE_ERROR_I2C               (-1)
#define MC34XX_RETCODE_ERROR_NULL_POINTER      (-2)
#define MC34XX_RETCODE_ERROR_STATUS            (-3)
#define MC34XX_RETCODE_ERROR_SETUP             (-4)
#define MC34XX_RETCODE_ERROR_GET_DATA          (-5)
#define MC34XX_RETCODE_ERROR_IDENTIFICATION    (-6)

/***********************************************
 *** CONFIGURATION
 ***********************************************/
#define MC34XX_BUF_SIZE    256

/***********************************************
 *** PRODUCT ID
 ***********************************************/
#define MC34XX_PCODE_3413     0x10
#define MC34XX_PCODE_3433     0x60
#define MC34XX_PCODE_3416     0x20
#define MC34XX_PCODE_3436     0x21

#endif    // END OF _MC3XXX_H_