/*
 * include/linux/spi/spidev.h
 *
 * Copyright (C) 2006 SWAPP
 *	Andrea Paterniani <a.paterniani@swapp-eng.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  */

#ifndef __DEV_INFO_H__
#define __DEV_INFO_H__

#ifdef CONFIG_TINNO_PRODUCT_INFO

#include <linux/types.h>
#include "external.h"

#define LOG_TAG_TINNO  "[PRODUCT_DEV_INFO]:" 
#define __TinnoFUN(f)   printk(KERN_ERR LOG_TAG_TINNO "~~~~~~~~~~~~ %s:%d ~~~~~~~~~\n", __func__,__LINE__)
#define tinnoklog(fmt, args...)    printk(KERN_ERR LOG_TAG_TINNO fmt, ##args)

#define show_content_len (128)


#define FULL_PRODUCT_DEVICE_CB(id, cb, args) \
    do { \
        full_product_device_info(id, NULL, cb, args); \
    } \
    while(0)

	
#define FULL_PRODUCT_DEVICE_INFO(id, info) \
    do { \
        full_product_device_info(id, info, NULL, NULL); \
    } \
    while(0)


#define FULL_PRODUCT_DEVICE_INFO_CAMERA( \
        invokeSocketIdx, \
        sensor_id, \
        sensor_name, \
        SensorFullWidth, \
        SensorFullHeight) \
do { \
    \
    if (sensor_id == 0) { \
        FULL_PRODUCT_DEVICE_CB(ID_MAIN_CAM, get_camera_info, "main"); \
    } \
    else if (sensor_id == 1) { \
        FULL_PRODUCT_DEVICE_CB(ID_SUB_CAM, get_camera_info, "sub"); \
    } \
    else if (sensor_id == 2) { \
        FULL_PRODUCT_DEVICE_CB(ID_MAIN2_CAM, get_camera_info, "main2"); \
    } \
    else if (sensor_id == 3) { \
        FULL_PRODUCT_DEVICE_CB(ID_SUB2_CAM, get_camera_info, "sub2"); \
    } \
    else { \
        FULL_PRODUCT_DEVICE_CB(ID_MAIN3_CAM, get_camera_info, "main3"); \
    } \
    sprintf(g_cam_info_struct[sensor_id].name, "%s", sensor_name); \
    g_cam_info_struct[sensor_id].id = invokeSocketIdx; \
    g_cam_info_struct[sensor_id].w = SensorFullWidth; \
    g_cam_info_struct[sensor_id].h = SensorFullHeight; \
} \
while(0)


typedef int (*FuncPtr)(char* buf, void *args);
	
typedef struct product_dev_info {
    char show[show_content_len];
    FuncPtr cb;
    void *args;
} product_info_arr;


enum product_dev_info_attribute {
    ID_LCD = 0,
    ID_TP = 1,
    ID_GYRO = 2,
    ID_GSENSOR = 3,
    ID_PSENSOR = 4,
    ID_MSENSOR = 5,
    ID_FLASH = 6,
    ID_BATTERY = 7,
    ID_MAIN_CAM = 8,
    ID_SUB_CAM = 9,
    ID_MAIN2_CAM = 10,
    ID_SUB2_CAM = 11,
    ID_MAIN3_CAM = 12,
    ID_FINGERPRINT = 13,
    ID_SECBOOT = 14, 
    ID_BL_LOCK_STATUS = 15, 
    ID_NFC = 16,
    ID_HALL = 17,
    ID_HW_BOARD_ID =18,
    
// add new..
    
    ID_MAX 
};

///////////////////////////////////////////////////////////////////////////////////////////
int full_product_device_info(int id, const char *info, FuncPtr cb, void *args);

///////////////////////////////////////////////////////////////////////////////////////////
#endif
#endif /* __FP_DRV_H */
