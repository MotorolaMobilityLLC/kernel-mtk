/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#define PFX "CAM_CAL"
#define pr_fmt(fmt) PFX "[%s] " fmt, __func__


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/of.h>
#include "cam_cal.h"
#include "cam_cal_define.h"
//#include "cam_cal_list.h"
#include <linux/dma-mapping.h>
#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif
#include "mot_bogota_sc820_mipi_raw_Sensor.h"
#include "mot_bogota_sc820_mipi_raw_otp.h"
#define MOT_BOGOTA_SC820_I2C_ID     0x6c
#define LOG_INF(format, args...)		pr_err(PFX "[%s] " format, __func__, ##args)


struct mot_bogota_sc820_otp_t mot_bogota_sc820_otp_info = {0};

static u16 read_cmos_sensor(u16 addr)
{
	u16 get_byte=0;
	char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };

	//kdSetI2CSpeed(400);

	iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, MOT_BOGOTA_SC820_I2C_ID);

	return get_byte;
}

static void write_cmos_sensor8(u16 addr, u16 para)
{
    char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};

	iWriteRegI2C(pu_send_cmd, 3, MOT_BOGOTA_SC820_I2C_ID);
}


static u8 CompareWriteAndRead(u16 uRegNum, BYTE * pWriteData, BYTE * pReadData, u16 size)
{
    int i;
    for ( i = 0; i < size; i++)
    {
        if (pWriteData[i] != pReadData[i])
        {
            pr_debug(PFX,"pWriteData[i]:0x%x  pReadData[i]:0x%x ", uRegNum + i, pWriteData[i], pReadData[i]);
            return 0;
        }
    }
    return 1;
}

static u16 mot_bogota_sc820_otp_read_group(u16 page, u16 addr, u8 *data, u16 length)
{
    int i;
    int times;
    size_t loop_time;
    BOOL re = TRUE;
    BYTE def = 0x00, busy_flag = 0x01;

    BYTE pRegData[2][390] = { 0 };
    BYTE threshold[3][3] = { {0x48,0x38,0x41},{0x48,0x18,0x41},{0x48,0x58,0x41} };

    for ( times = 0; times < 2; times++)
    {
        write_cmos_sensor8(0x36b0, threshold[times][0]);
        write_cmos_sensor8(0x36b1, threshold[times][1]);
        write_cmos_sensor8(0x36b2, threshold[times][2]);
        write_cmos_sensor8(0x4408, 0x80 + (page - 1) * 0x02);
        write_cmos_sensor8(0x4409, 0x00);
        write_cmos_sensor8(0x440a, 0x81 + (page - 1) * 0x02);
        write_cmos_sensor8(0x440b, 0xff);
        write_cmos_sensor8(0x4401, 0x13);
        write_cmos_sensor8(0x4412, 0x03 + (page - 2) * 0x02);
        write_cmos_sensor8(0x4407, 0x00);

        write_cmos_sensor8(0x4400, 0x11);
        mdelay(10);

        for ( loop_time = 0; loop_time < 1000; loop_time++)
        {
            mdelay(5);
            def = read_cmos_sensor(0x4420);//[0]busy,0 ok//[1]otp,0 ok
            busy_flag = def & 0x1;
            if (0 == busy_flag) break;
        }


        if (busy_flag)
        {
            re = FALSE;
            goto READ_CLOCK_END;
        }

        for ( i = 0; i < length; i++)
        {
            pRegData[times][i] = read_cmos_sensor(addr+i);
            //pr_debug(PFX,"addr = 0x%x, data = 0x%x\n", addr+i, pRegData[times][i]);
        }
    }

    if (CompareWriteAndRead(addr, pRegData[0], pRegData[1], length))
    {
        //size = 390;
        memcpy(data, pRegData[0], length);
    }
    else
    {
        re = -2;
    }

READ_CLOCK_END:

    if (2 == page)
    {
        write_cmos_sensor8(0x0100, 0x00);
        write_cmos_sensor8(0x4424, 0x01);
        write_cmos_sensor8(0x4408, 0x00);
        write_cmos_sensor8(0x4409, 0x00);
        write_cmos_sensor8(0x440a, 0x01);
        write_cmos_sensor8(0x440b, 0xff);
        write_cmos_sensor8(0x4401, 0x13);
        write_cmos_sensor8(0x4412, 0x01);
        write_cmos_sensor8(0x4407, 0x0e);
        //WriteIIC(0x3106, 0x01);
        write_cmos_sensor8(0x363c, 0x8c);
        write_cmos_sensor8(0x36b0, 0x48);
        write_cmos_sensor8(0x36b1, 0x38);
        write_cmos_sensor8(0x36b2, 0x41);
        write_cmos_sensor8(0x0100, 0x01);
        mdelay(100);
    }

    return re;
}

static int mot_bogota_sc820_iReadData(u16 page, unsigned int ui4_offset, unsigned int ui4_length, unsigned char *pinputdata)
{
    int i4RetValue = 0;
    int i4ResidueDataLength;
    u32 u4CurrentOffset;
    u8 *pBuff;

    pr_debug(PFX,"ui4_offset = 0x%x, ui4_length = %d \n", ui4_offset, ui4_length);

    i4ResidueDataLength = (int)ui4_length;
    u4CurrentOffset = ui4_offset;
    pBuff = pinputdata;

    i4RetValue = mot_bogota_sc820_otp_read_group(page, (u16) u4CurrentOffset, pBuff, i4ResidueDataLength);
    if (i4RetValue != 1) {
        pr_debug(PFX,"I2C iReadData failed!!\n");
        return -1;
    }

    return 0;
}

static bool mot_bogota_sc820_param_checksum(u8 *buf, unsigned int size, u8 checksum)
{
    int i, sum = 0;

    for (i = 0; i < size; i++)
    {
        sum += buf[i];
        //pr_debug(PFX,"buf[%d] = 0x%x %d", i, buf[i], buf[i]);
    }

    if ((sum % 255 + 1) != checksum)
    {
        LOG_INF("checksum fail size = %d sum=%d sum-in-eeprom=%d", size, sum % 255 + 1, checksum);
        return false;
    }
    LOG_INF("checksum success size = %d sum=%d sum-in-eeprom=%d", size, sum % 255 + 1, checksum);
    return true;
}

static bool mot_bogota_sc820_read_module_info(u8 moduleflag)
{
    bool ret = false;

    pr_debug(PFX,"--------------mot_bogota_sc820 module info read begin------------\n");
    if (moduleflag == 1) {
        mot_bogota_sc820_iReadData(MODULE_GROUP_INFO_PAGE, MODULE_GROUP1_INFO_ADDR, MODULE_INFO_LENGTH, &mot_bogota_sc820_otp_info.module_param[0]);
        mot_bogota_sc820_iReadData(MODULE_GROUP_INFO_PAGE, MODULE_GROUP1_CHECKSUM, 1, &mot_bogota_sc820_otp_info.module_checksum);
    } else if (moduleflag  == 2) {
        mot_bogota_sc820_iReadData(MODULE_GROUP_INFO_PAGE, MODULE_GROUP2_INFO_ADDR, MODULE_INFO_LENGTH, &mot_bogota_sc820_otp_info.module_param[0]);
        mot_bogota_sc820_iReadData(MODULE_GROUP_INFO_PAGE, MODULE_GROUP2_CHECKSUM, 1, &mot_bogota_sc820_otp_info.module_checksum);
    } else {
        pr_debug(PFX,"--------------mot_bogota_sc820 module info read failed------------\n");
    }
    pr_debug(PFX,"--------------mot_bogota_sc820 module info read end------------\n");
    ret = mot_bogota_sc820_param_checksum(&mot_bogota_sc820_otp_info.module_param[0], MODULE_INFO_LENGTH, mot_bogota_sc820_otp_info.module_checksum);
    if (ret) {
        LOG_INF("--------------mot_bogota_sc820 module info checksum success------------\n");
    }
    return ret;
}

static bool mot_bogota_sc820_read_awb_info(u8 moduleflag)
{
    bool ret = false;
    pr_debug(PFX,"--------------mot_bogota_sc820 awb info read begin------------\n");
    if (moduleflag  == 1) {
        mot_bogota_sc820_iReadData(AWB_GROUP_INFO_PAGE, AWB_GROUP1_INFO_ADDR, AWB_INFO_LENGTH, &mot_bogota_sc820_otp_info.awb_param[0]);
        mot_bogota_sc820_iReadData(AWB_GROUP_INFO_PAGE, AWB_GROUP1_CHECKSUM, 1, &mot_bogota_sc820_otp_info.awb_checksum);
    } else if (moduleflag == 2) {
        mot_bogota_sc820_iReadData(AWB_GROUP_INFO_PAGE, AWB_GROUP2_INFO_ADDR, AWB_INFO_LENGTH, &mot_bogota_sc820_otp_info.awb_param[0]);
        mot_bogota_sc820_iReadData(AWB_GROUP_INFO_PAGE, AWB_GROUP2_CHECKSUM, 1, &mot_bogota_sc820_otp_info.awb_checksum);
    } else {
        pr_debug(PFX,"--------------mot_bogota_sc820 awb info read failed------------\n");
    }
    pr_debug(PFX,"--------------mot_bogota_sc820 awb info read end------------\n");
    ret = mot_bogota_sc820_param_checksum(&mot_bogota_sc820_otp_info.awb_param[0], AWB_INFO_LENGTH, mot_bogota_sc820_otp_info.awb_checksum);
    if (ret) {
        LOG_INF("--------------mot_bogota_sc820 awb info checksum success------------\n");
    }
    return ret;
}

static bool mot_bogota_sc820_read_lsc_info(u8 moduleflag)
{
    bool ret = false;
    int idex = 0;

    u8 *pBuff;
    pr_debug(PFX,"--------------mot_bogota_sc820 lsc info read begin------------\n");
    if (moduleflag  == 1) {
        pr_debug(PFX,"--------------mot_bogota_sc820 lsc info read part1 idex %d------------\n", idex);
        pBuff = &mot_bogota_sc820_otp_info.lsc_param[idex];
        mot_bogota_sc820_iReadData(LSC_GROUP1_PART1_INFO_PAGE, LSC_GROUP1_PART1_INFO_ADDR, LSC_GROUP1_PART1_INFO_LENGTH, pBuff);

        idex = idex + LSC_GROUP1_PART1_INFO_LENGTH;
        pBuff = &mot_bogota_sc820_otp_info.lsc_param[idex];
        pr_debug(PFX,"--------------mot_bogota_sc820 lsc info read part2 idex %d ------------\n", idex);
        mot_bogota_sc820_iReadData(LSC_GROUP1_PART2_INFO_PAGE, LSC_GROUP1_PART2_INFO_ADDR, LSC_GROUP1_PART2_INFO_LENGTH,  pBuff/*&mot_bogota_sc820_otp_info.lsc_param[idex]*/);

        idex = idex + LSC_GROUP1_PART2_INFO_LENGTH;
        pr_debug(PFX,"--------------mot_bogota_sc820 lsc info read part3 idex %d ------------\n", idex);
        pBuff = &mot_bogota_sc820_otp_info.lsc_param[idex];
        mot_bogota_sc820_iReadData(LSC_GROUP1_PART3_INFO_PAGE, LSC_GROUP1_PART3_INFO_ADDR, LSC_GROUP1_PART3_INFO_LENGTH, pBuff/*&mot_bogota_sc820_otp_info.lsc_param[idex]*/);

        idex = idex + LSC_GROUP1_PART3_INFO_LENGTH;
        pr_debug(PFX,"--------------mot_bogota_sc820 lsc info read part4 idex %d ------------\n", idex);
        pBuff = &mot_bogota_sc820_otp_info.lsc_param[idex];
        mot_bogota_sc820_iReadData(LSC_GROUP1_PART4_INFO_PAGE, LSC_GROUP1_PART4_INFO_ADDR, LSC_GROUP1_PART4_INFO_LENGTH, pBuff/*&mot_bogota_sc820_otp_info.lsc_param[idex]*/);

        idex = idex + LSC_GROUP1_PART4_INFO_LENGTH;
        pr_debug(PFX,"--------------mot_bogota_sc820 lsc info read part5 idex %d ------------\n", idex);
        pBuff = &mot_bogota_sc820_otp_info.lsc_param[idex];
        mot_bogota_sc820_iReadData(LSC_GROUP1_PART5_INFO_PAGE, LSC_GROUP1_PART5_INFO_ADDR, LSC_GROUP1_PART5_INFO_LENGTH, pBuff/*&mot_bogota_sc820_otp_info.lsc_param[idex]*/);

        idex = idex + LSC_GROUP1_PART5_INFO_LENGTH;
        pr_debug(PFX,"--------------mot_bogota_sc820 lsc info read part5 idex %d ------------\n", idex);
        pBuff = &mot_bogota_sc820_otp_info.lsc_param[idex];
        mot_bogota_sc820_iReadData(LSC_GROUP1_PART6_INFO_PAGE, LSC_GROUP1_PART6_INFO_ADDR, LSC_GROUP1_PART6_INFO_LENGTH, pBuff/*&mot_bogota_sc820_otp_info.lsc_param[idex]*/);
        /*for (int i = 0; i < LSC_INFO_LENGTH; i++)
        {
            LOG_INF("summation index %d , data = 0x%x\n", i, mot_bogota_sc820_otp_info.lsc_param[i]);
        }*/

        pr_debug(PFX,"--------------mot_bogota_sc820 lsc info read checksum ------------\n");
        mot_bogota_sc820_iReadData(LSC_GROUP1_PART6_INFO_PAGE, LSC_GROUP1_CHECKSUM, 1, &mot_bogota_sc820_otp_info.lsc_checksum);
    } else if (moduleflag  == 2) {
        pr_debug(PFX,"--------------mot_bogota_sc820 lsc info read part1 ------------\n");
        mot_bogota_sc820_iReadData(LSC_GROUP2_PART1_INFO_PAGE, LSC_GROUP2_PART1_INFO_ADDR, LSC_GROUP2_PART1_INFO_LENGTH, &mot_bogota_sc820_otp_info.lsc_param[idex]);

        idex += LSC_GROUP2_PART1_INFO_LENGTH;
        pr_debug(PFX,"--------------mot_bogota_sc820 lsc info read part2 ------------\n");
        mot_bogota_sc820_iReadData(LSC_GROUP2_PART2_INFO_PAGE, LSC_GROUP2_PART2_INFO_ADDR, LSC_GROUP2_PART2_INFO_LENGTH, &mot_bogota_sc820_otp_info.lsc_param[idex]);

        idex += LSC_GROUP2_PART2_INFO_LENGTH;
        pr_debug(PFX,"--------------mot_bogota_sc820 lsc info read part3 ------------\n");
        mot_bogota_sc820_iReadData(LSC_GROUP2_PART3_INFO_PAGE, LSC_GROUP2_PART3_INFO_ADDR, LSC_GROUP2_PART3_INFO_LENGTH, &mot_bogota_sc820_otp_info.lsc_param[idex]);

        idex += LSC_GROUP2_PART3_INFO_LENGTH;
        pr_debug(PFX,"--------------mot_bogota_sc820 lsc info read part4 ------------\n");
        mot_bogota_sc820_iReadData(LSC_GROUP2_PART4_INFO_PAGE, LSC_GROUP2_PART4_INFO_ADDR, LSC_GROUP2_PART4_INFO_LENGTH, &mot_bogota_sc820_otp_info.lsc_param[idex]);

        idex += LSC_GROUP2_PART4_INFO_LENGTH;
        pr_debug(PFX,"--------------mot_bogota_sc820 lsc info read part5 ------------\n");
        mot_bogota_sc820_iReadData(LSC_GROUP2_PART5_INFO_PAGE, LSC_GROUP2_PART5_INFO_ADDR, LSC_GROUP2_PART5_INFO_LENGTH, &mot_bogota_sc820_otp_info.lsc_param[idex]);

        pr_debug(PFX,"--------------mot_bogota_sc820 lsc info read checksum ------------\n");
        mot_bogota_sc820_iReadData(LSC_GROUP2_PART5_INFO_PAGE, LSC_GROUP2_CHECKSUM, 1, &mot_bogota_sc820_otp_info.lsc_checksum);
    } else {
        pr_debug(PFX,"--------------mot_bogota_sc820 lsc info read failed------------\n");
    }
    pr_debug(PFX,"--------------mot_bogota_sc820 lsc info read end------------\n");
    ret = mot_bogota_sc820_param_checksum(&mot_bogota_sc820_otp_info.lsc_param[0], LSC_INFO_LENGTH, mot_bogota_sc820_otp_info.lsc_checksum);
    if (ret) {
        LOG_INF("--------------mot_bogota_sc820 lsc info checksum success------------\n");
    }
    return ret;
}

void read_mot_bogota_sc820_otp_data(void)
{
    u8 moduleflag =0;
    u8 value =0;
    bool checksum_module = false;
    bool checksum_awb = false;
    bool checksum_lsc = false;

    pr_debug(PFX,"mot_bogota_sc820 moduleflag read begin");
    mot_bogota_sc820_iReadData(MODULE_GROUP_INFO_PAGE, OTP_MODULE_FLAG, 1, &value);
    if (value == 1) {
         moduleflag = 1;
    } else if (value == 19) {
        pr_debug(PFX,"mot_bogota_sc820 group flag = 0x%x", value);
            moduleflag = 2;
    }
    pr_debug(PFX,"mot_bogota_sc820 moduleflag = 0x%x end", moduleflag);
    if (moduleflag != 1 && moduleflag != 2) {
        pr_debug(PFX,"mot_bogota_sc820 invalid moduleflag = 0x%x", moduleflag);
        return;
    }

    checksum_module = mot_bogota_sc820_read_module_info(moduleflag);
    checksum_awb = mot_bogota_sc820_read_awb_info(moduleflag);
    checksum_lsc = mot_bogota_sc820_read_lsc_info(moduleflag);

    if (true == (checksum_module & checksum_awb & checksum_lsc))
    {
        LOG_INF("----------------mot_bogota_sc820 otp info check success----------------");
    }
    else
    {
        LOG_INF("----------------mot_bogota_sc820 otp info check fail-------------------");
    }
}

unsigned int mot_bogota_sc820_read_region(struct i2c_client *client, unsigned int addr,
                                unsigned char *data, unsigned int size)
{
    unsigned char *dataTmp = data;

    pr_err("mot_bogota_sc820 otp region addr = 0x%x, size = %d\n", addr, size);
    if (addr == 0x1 && size == 1) {//0xff
        *(u32 *)data = 0x00000006;
    } else if (addr == 0x0 && size == 1917) {
        unsigned int totalSize = sizeof(mot_bogota_sc820_otp_info.module_flag) +
                                 sizeof(mot_bogota_sc820_otp_info.module_param) +
                                 sizeof(mot_bogota_sc820_otp_info.module_checksum) +
                                 sizeof(mot_bogota_sc820_otp_info.awb_param) +
                                 sizeof(mot_bogota_sc820_otp_info.awb_checksum) +
                                 sizeof(mot_bogota_sc820_otp_info.lsc_param) +
                                 sizeof(mot_bogota_sc820_otp_info.lsc_checksum);
        pr_err("mot_bogota_sc820 otp region addr = 0x%x, size = %d  totalSize=%d \n", addr, size, totalSize);
        if (size == totalSize) {
            data[0] = mot_bogota_sc820_otp_info.module_flag;

            dataTmp += sizeof(mot_bogota_sc820_otp_info.module_flag);

            memcpy(dataTmp, mot_bogota_sc820_otp_info.module_param, sizeof(mot_bogota_sc820_otp_info.module_param));
            dataTmp += sizeof(mot_bogota_sc820_otp_info.module_param);

            data[23] = mot_bogota_sc820_otp_info.module_checksum;
            dataTmp += sizeof(mot_bogota_sc820_otp_info.module_checksum);

            memcpy(dataTmp, mot_bogota_sc820_otp_info.awb_param, sizeof(mot_bogota_sc820_otp_info.awb_param));
            dataTmp += sizeof(mot_bogota_sc820_otp_info.awb_param);

            data[47] = mot_bogota_sc820_otp_info.awb_checksum;
            dataTmp += sizeof(mot_bogota_sc820_otp_info.awb_checksum);

            memcpy(dataTmp, mot_bogota_sc820_otp_info.lsc_param, sizeof(mot_bogota_sc820_otp_info.lsc_param));
            dataTmp += sizeof(mot_bogota_sc820_otp_info.lsc_param);

            data[totalSize - 1] = mot_bogota_sc820_otp_info.lsc_checksum;
        } else {
            pr_err("mot_bogota_sc820 otp size != totalSize");
            size = totalSize;
        }
    } else if (addr == 40 && size == 23) { //read single awb data
        memcpy(data,(mot_bogota_sc820_otp_info.awb_param), size);
        pr_err("add = 0x%x, read awb\n",addr);
    } else if (size >=1868 && size < 2048 && addr == 69) {
        memcpy(data, mot_bogota_sc820_otp_info.lsc_param, size);
        pr_err("add = 0x%x, read lsc\n",addr);
    } else if (addr == 1937 && size == 1) {
        *(u32 *)data = mot_bogota_sc820_otp_info.lsc_checksum;
        pr_err("add = 0x%x, read lsc_checksum = %x\n",addr, *(u32 *)data);
    } else if (addr == 68 && size == 1) {
        *(u32 *)data = mot_bogota_sc820_otp_info.awb_checksum;
        pr_err("add = 0x%x, read awb_checksum = %x\n",addr, *(u32 *)data);
    } else{
        pr_err("mot_bogota_sc820 otp add = 0x%x, size = %d ,read error !!!\n",addr,size);
    }
    return size;
}
EXPORT_SYMBOL(mot_bogota_sc820_read_region);
