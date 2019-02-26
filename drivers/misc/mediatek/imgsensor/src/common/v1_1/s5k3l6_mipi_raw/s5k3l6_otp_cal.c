/**
Copyright(C) 2015 Transsion Inc
*/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/proc_fs.h>
#include <linux/dma-mapping.h>

#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "../../../../common/v1/dualcam_utils.h"

#define CALI_PFX "[S5K3L6_CALI]"
#define CALI_INF(fmt, args...)   pr_debug(CALI_PFX "[%s] " fmt, __FUNCTION__, ##args)
#define CALI_ERR(fmt, args...)   pr_err(CALI_PFX "[%s] " fmt, __FUNCTION__, ##args)

#define BYTE                unsigned char

#define EEPROM              BL24SA64
#define EEPROM_READ_ID    	(0xA1)
#define EEPROM_WRITE_ID    	(0xA0)

#define MAX_OFFSET          (0xffff)
#define CALI_FLAG_OFFSET  (0x1997)  //spi
#define CALI_DATA_OFFSET (0x1998)
#define AESYNC_DATA_OFFSET (0x01A3)

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

static bool selective_read_eeprom(u16 addr, BYTE* data)
{
	char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	if(addr > MAX_OFFSET)
		return false;

	if(iReadRegI2C(pu_send_cmd, 2, (u8*)data, 1, EEPROM_READ_ID)<0)
		return false;

	return true;
}

static bool selective_write_eeprom(u16 addr, BYTE* data)
{
	char pu_send_cmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(*data & 0xFF)};
	if(addr > MAX_OFFSET)
		return false;

	if(iWriteRegI2C(pu_send_cmd, 3, EEPROM_WRITE_ID)<0)
		return false;
    mdelay(5);
	return true;
}

static int write_eeprom_s5k3l6(u16 addr, BYTE* data, u32 size )
{
	int i = 0;
	BYTE flag = 0x00;
	int offset = addr;

	/*switched off write protect*/
	selective_write_eeprom(0x8000, &flag);
	mdelay(1);
	/*check again*/
	selective_read_eeprom(0x8000, &flag);
	if(flag != 0x00)
	{
		CALI_ERR("close write protect failed");
		return -1;
	}

	for(i = 0; i < size; i++)
	{
		if(!selective_write_eeprom(offset, &data[i]))
		{
            CALI_ERR("write failed, now/total:(%d)/(%d)", i, size);
			return -1;
		}
		//CALI_INF("write_eeprom 0x%x 0x%x\n",offset, data[i]);
		offset++;
		msleep(1);
	}
	return i;
}

static int read_eeprom_s5k3l6(u16 addr, BYTE* data, u32 size )
{
	int i = 0;
	int offset = addr;

	for(i = 0; i < size; i++)
	{
		if(!selective_read_eeprom(offset, &data[i]))
		{
            CALI_ERR("read failed, now/total:(%d)/(%d)", i, size);
			return -1;
		}
		//CALI_INF("read_eeprom 0x%x 0x%x\n",offset, data[i]);
		offset++;
	}
	return i;
}

int dump_dualcam_data_s5k3l6(ENUM_DUALCAM_UTILS_EEPROM_OPS_TAG_NAME tag, char *buf, int buf_len_max)
{
    /*return dumped buf len or error code*/
    int buf_len = 0;

    if(buf == NULL
        || buf_len_max <= 0)
    {
        CALI_ERR("empty buffer ,tag (%d)\n", tag);
        return -1;
    }

    switch(tag)
    {
        case verity_flag:
            buf_len = read_eeprom_s5k3l6(CALI_FLAG_OFFSET, buf, buf_len_max);
            break;
        case calibration_data:
            buf_len = read_eeprom_s5k3l6(CALI_DATA_OFFSET, buf, buf_len_max);
            break;
        case aesync_data:
            buf_len = read_eeprom_s5k3l6(AESYNC_DATA_OFFSET, buf, buf_len_max);
            break;
        default:
            CALI_ERR("get invalid tag (%d)\n", tag);
            return -1;
    }

    CALI_INF("handle tag (%d), return %d\n", tag, buf_len);
    return buf_len;
}

int store_dualcam_data_s5k3l6(ENUM_DUALCAM_UTILS_EEPROM_OPS_TAG_NAME tag, char *buf, int buf_len_max)
{
    /*return stored buf len or error code*/
    int buf_len = 0;

    if(buf == NULL
        || buf_len_max <= 0)
    {
        CALI_ERR("empty buffer ,tag (%d)\n", tag);
        return -1;
    }

    switch(tag)
    {
        case verity_flag:
            buf_len = write_eeprom_s5k3l6(CALI_FLAG_OFFSET, buf, buf_len_max);
            mdelay(1);
            break;
        case calibration_data:
            buf_len = write_eeprom_s5k3l6(CALI_DATA_OFFSET, buf, buf_len_max);
            mdelay(1);
            break;
        default:
            CALI_ERR("get invalid tag (%d)\n", tag);
            return -1;
    }

    CALI_INF("handle tag (%d), return %d\n", tag, buf_len);
    return buf_len;
}
