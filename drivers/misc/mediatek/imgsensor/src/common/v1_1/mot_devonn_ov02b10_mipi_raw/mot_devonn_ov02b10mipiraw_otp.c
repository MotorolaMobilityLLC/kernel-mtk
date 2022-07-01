/*
 * Copyright (C) 2021 lucas (guoqiang8@lenovo.com).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
  ****************************************************************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>
#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "mot_devonn_ov02b10mipiraw_otp.h"
#define PFX "[MOTO_EEPROM ov02b10-otp]"

static int m_mot_camera_debug = 1;
#define LOG_INF(format, args...)        do { if (m_mot_camera_debug   ) { pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args); } } while(0)
#define I2C_ADDR 0x78
unsigned char ov02b10_otp_data[OV02B10_OTP_SIZE] = {0};
static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;
    char pu_send_cmd[1] = {(char)(addr & 0xFF)};
    iReadRegI2C(pu_send_cmd, 1, (u8*)&get_byte, 1, I2C_ADDR);
    return get_byte;
}
static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
    char pu_send_cmd[2] = {(char)(addr & 0xFF), (char)(para & 0xFF)};
    iWriteRegI2C(pu_send_cmd, 2, I2C_ADDR);
}
static int32_t eeprom_util_check_crc16(uint8_t *data, uint32_t size, uint32_t ref_crc)
{
    int32_t crc_match = 0;
    uint8_t crc = 0x00;
    uint32_t i;
    uint32_t tmp = 0;
    /* Calculate both methods of CRC since integrators differ on
    * how CRC should be calculated. */
    for (i = 0; i < size; i++) {
        tmp += data[i];
    }
    crc = tmp%0xff + 1;
    if (crc == ref_crc)
        crc_match = 1;
    LOG_INF("REF_CRC 0x%x CALC CRC 0x%x  matches? %d\n",
        ref_crc, crc, crc_match);
    return crc_match;
}
static calibration_status_t DEVONN_OV02B10_check_awb_data(void *data)
{
    unsigned char *data_awb = data; //add flag and checksum value
    if(((data_awb[0]&0xC0)>>6) == 0x01){ //Bit[7:6] 01:Valid 11:Invalid
        LOG_INF("awb data is group1\n");
        if(!eeprom_util_check_crc16(&data_awb[0],
        DEVONN_OV02B10_OTP_CRC_AWB_GROUP1_CAL_SIZE,
        data_awb[7])) {
            LOG_INF("AWB CRC Fails!");
            return CRC_FAILURE;
        }
    } else if(((data_awb[0]&0x30)>>4) == 0x01){ //Bit[5:4] 01:Valid 11:Invalid
        LOG_INF("awb data is group2\n");
        if(!eeprom_util_check_crc16(&data_awb[8],
        DEVONN_OV02B10_OTP_CRC_AWB_GROUP2_CAL_SIZE,
        data_awb[14])) {
            LOG_INF("AWB CRC Fails!");
            return CRC_FAILURE;
        }
    } else {
        LOG_INF("ov02b10 OTP has no awb data\n");
        return CRC_FAILURE;
    }
    LOG_INF("AWB CRC Pass");
    return NO_ERRORS;
}
void DEVONN_OV02B10_eeprom_format_calibration_data(void *data, ov02b10_calibration_status_t *ov02b10_cal_info)
{
    if (NULL == data) {
        LOG_INF("data is NULL");
        return;
    }
    ov02b10_cal_info->mnf_status            = NO_ERRORS;
    ov02b10_cal_info->af_status             = NO_ERRORS;
    ov02b10_cal_info->awb_status            = DEVONN_OV02B10_check_awb_data(data);
    ov02b10_cal_info->lsc_status            = NO_ERRORS;
    ov02b10_cal_info->pdaf_status           = NO_ERRORS;
    ov02b10_cal_info->dual_status           = NO_ERRORS;
    LOG_INF("status mnf:%d, af:%d, awb:%d, lsc:%d, pdaf:%d, dual:%d",
        ov02b10_cal_info->mnf_status, ov02b10_cal_info->af_status, ov02b10_cal_info->awb_status,
        ov02b10_cal_info->lsc_status, ov02b10_cal_info->pdaf_status, ov02b10_cal_info->dual_status);
}
int ov02b10_read_data_from_otp(void)
{
    int i=0;
    LOG_INF("ov02b10_read_data_from_otp -E");
    write_cmos_sensor(0xfd, 0x06);
    write_cmos_sensor(0x21, 0x00);
    write_cmos_sensor(0x2f, 0x01);
    for(i=0;i<OV02B10_OTP_SIZE;i++)
    {
        ov02b10_otp_data[i]=read_cmos_sensor(i);
    }
    LOG_INF("ov02b10_read_data_from_otp -X");
    return 0;
}
unsigned int mot_ov02b10_read_region(struct i2c_client *client, unsigned int addr,
            unsigned char *data, unsigned int size)
{
    unsigned int ret = 0;
    unsigned char *otp_data = ov02b10_otp_data;
    LOG_INF ("eeprom read data addr = 0x%x size = %d\n",addr,size);
    if (size > OV02B10_OTP_SIZE)
        size = OV02B10_OTP_SIZE;
    memcpy(data, otp_data + addr,size);
    ret = size;
    return ret;
}
EXPORT_SYMBOL(mot_ov02b10_read_region);
