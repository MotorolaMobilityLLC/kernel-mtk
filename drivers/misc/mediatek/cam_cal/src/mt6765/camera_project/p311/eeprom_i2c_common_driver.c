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
#include "cam_cal_list.h"
#include <linux/dma-mapping.h>
#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif


static DEFINE_SPINLOCK(g_spinLock);


/************************************************************
 * I2C read function (Common)
 ************************************************************/
static struct i2c_client *g_pstI2CclientG;

/* add for linux-4.4 */
#ifndef I2C_WR_FLAG
#define I2C_WR_FLAG		(0x1000)
#define I2C_MASK_FLAG	(0x00ff)
#endif

static int Read_I2C_CAM_CAL(u16 a_u2Addr, u32 ui4_length, u8 *a_puBuff)
{
	int i4RetValue = 0;
	char puReadCmd[2] = { (char)(a_u2Addr >> 8), (char)(a_u2Addr & 0xFF) };


	if (ui4_length > 8) {
		pr_debug("exceed I2c-mt65xx.c 8 bytes limitation\n");
		return -1;
	}
	spin_lock(&g_spinLock);
	g_pstI2CclientG->addr =
		g_pstI2CclientG->addr & (I2C_MASK_FLAG | I2C_WR_FLAG);
	spin_unlock(&g_spinLock);

	i4RetValue = i2c_master_send(g_pstI2CclientG, puReadCmd, 2);
	if (i4RetValue != 2) {
		pr_debug("I2C send read address failed!!\n");
		return -1;
	}

	i4RetValue = i2c_master_recv(g_pstI2CclientG,
						(char *)a_puBuff, ui4_length);
	if (i4RetValue != ui4_length) {
		pr_debug("I2C read data failed!!\n");
		return -1;
	}

	spin_lock(&g_spinLock);
	g_pstI2CclientG->addr = g_pstI2CclientG->addr & I2C_MASK_FLAG;
	spin_unlock(&g_spinLock);
	return 0;
}

int iReadData_CAM_CAL(unsigned int ui4_offset,
	unsigned int ui4_length, unsigned char *pinputdata)
{
	int i4RetValue = 0;
	int i4ResidueDataLength;
	u32 u4IncOffset = 0;
	u32 u4CurrentOffset;
	u8 *pBuff;

	if (ui4_offset + ui4_length >= 0x2000) {
		pr_debug
		    ("Read Error!! not supprt address >= 0x2000!!\n");
		return -1;
	}

	i4ResidueDataLength = (int)ui4_length;
	u4CurrentOffset = ui4_offset;
	pBuff = pinputdata;
	do {
		if (i4ResidueDataLength >= 8) {
			i4RetValue = Read_I2C_CAM_CAL(
				(u16) u4CurrentOffset, 8, pBuff);
			if (i4RetValue != 0) {
				pr_debug("I2C iReadData failed!!\n");
				return -1;
			}
			u4IncOffset += 8;
			i4ResidueDataLength -= 8;
			u4CurrentOffset = ui4_offset + u4IncOffset;
			pBuff = pinputdata + u4IncOffset;
		} else {
			i4RetValue =
			    Read_I2C_CAM_CAL(
			    (u16) u4CurrentOffset, i4ResidueDataLength, pBuff);
			if (i4RetValue != 0) {
				pr_debug("I2C iReadData failed!!\n");
				return -1;
			}
			u4IncOffset += 8;
			i4ResidueDataLength -= 8;
			u4CurrentOffset = ui4_offset + u4IncOffset;
			pBuff = pinputdata + u4IncOffset;
			/* break; */
		}
	} while (i4ResidueDataLength > 0);



	return 0;
}

unsigned int Common_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size)
{
	g_pstI2CclientG = client;
	if (iReadData_CAM_CAL(addr, size, data) == 0)
		return size;
	else
		return 0;
}



#define I2C_RETRY_NUMBER 3
static int i2c_write(struct i2c_client *client, char *writebuf, int writelen)
{
    int ret = 0;
    int i = 0;

    if (client == NULL) {
        pr_err("[IIC][%s]i2c_client==NULL!", __func__);
        return -EINVAL;
    }

    if (writelen > 0) {
        struct i2c_msg msgs[] = {
            {
                .addr = client->addr,
                .flags = 0,
                .len = writelen,
                .buf = writebuf,
            },
        };
        for (i = 0; i < I2C_RETRY_NUMBER; i++) {
            ret = i2c_transfer(client->adapter, msgs, 1);
            if (ret < 0) {
                pr_err("[IIC]: i2c_transfer(write) error, ret=%d!!", ret);
            } else
                break;
        }
    }

    return ret;
}


static int i2c_write_reg(struct i2c_client *client, unsigned int regaddr, u8 regvalue)
{
    u8 buf[3] = {0};

    buf[0] = regaddr >> 8;
    buf[1] = regaddr & 0xff;
    buf[2] = regvalue;
    return i2c_write(client, buf, sizeof(buf));
}

static int i2c_write_page(struct i2c_client *client, unsigned int regaddr,  unsigned char *regvalue)
{
    u8 buf[2 + 32] = {0};
    int i;

    buf[0] = regaddr >> 8;
    buf[1] = regaddr & 0xff;
  
    for(i = 0; i < 32; i++)  
	buf[i + 2] = regvalue[i];

    return i2c_write(client, buf, 32 + 2);
}

#define LENGTH  2048
unsigned int Common_write_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size)
{
	int i, j; 
	int ret = LENGTH;

        u8 *pBuff = NULL;

	if(size != LENGTH) {
		pr_err("size != 2048\n");
		return 0;	
	}

        pBuff = kzalloc(LENGTH, GFP_KERNEL);
	if (pBuff == NULL) {
		pr_err("ioctl allocate pBuff mem failed\n");
		return 0;
	}

	g_pstI2CclientG = client;//for i2c  read

	addr = 0xce0;
	
	i2c_write_reg(client, 0x8000, 0x00);
	mdelay(30);

	j = LENGTH >> 5;
	
	for(i = 0; i < j; i++) {
		i2c_write_page(client, addr + i * 32, &data[i * 32]);
		mdelay(30);
	}

	iReadData_CAM_CAL(addr, LENGTH, pBuff); 

	for(i = 0; i < LENGTH; i++) {
		if(pBuff[i] != data[i]) {
			pr_err("!err   pBuff[%d] = %d, data[%d] = %d\n", i, pBuff[i], i, data[i]);
			ret = 0;
		}
	}
	i2c_write_reg(client, 0x8000, 0x0e);
	kfree(pBuff);	
	pr_err("3d calibration write ok!");	
	return ret;
}
