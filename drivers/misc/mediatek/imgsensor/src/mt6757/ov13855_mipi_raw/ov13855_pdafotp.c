#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/slab.h>


#define PFX "OV13855_pdafotp"
#define LOG_INF(format, args...)	pr_err(PFX "[%s] " format, __FUNCTION__, ##args)
#include "kd_camera_typedef.h"
#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern void kdSetI2CSpeed(u16 i2cSpeed);
//extern int iBurstWriteReg_multi(u8 *pData, u32 bytes, u16 i2cId, u16 transfer_length);
extern int iMultiReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId, u8 number);


#define USHORT             unsigned short
#define BYTE               unsigned char
#define Sleep(ms) mdelay(ms)

#define OV13855_EEPROM_READ_ID  0xA0
#define OV13855_EEPROM_WRITE_ID   0xA0
#define OV13855_I2C_SPEED        100
#define OV13855_MAX_OFFSET		0xFFFF

#define DATA_SIZE 2048
BYTE ov13855_eeprom_data[DATA_SIZE]= {0};
static bool get_done = false;
static int last_size = 0;
static int last_offset = 0;


static bool selective_read_eeprom(kal_uint16 addr, BYTE* data)
{
	char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
    if(addr > OV13855_MAX_OFFSET)
        return false;
//	kdSetI2CSpeed(OV13855_I2C_SPEED);
	if(iReadRegI2C(pu_send_cmd, 2, (u8*)data, 1, OV13855_EEPROM_WRITE_ID)<0)
		return false;
    return true;
}

static bool _read_ov13855_eeprom(kal_uint16 addr, BYTE* data, kal_uint32 size ){
	int i = 0;
	int offset = addr;
	printk("huangsh4 _read_ov13855_eeprom\n");
	for(i = 0; i < size; i++) {
		if(!selective_read_eeprom(offset, &data[i])){
			return false;
		}
		LOG_INF("huangsh4 read_eeprom 0x%0x %d=== %0x\n",offset, data[i],data[i]);
		offset++;
	}
	get_done = true;
	last_size = size;
	last_offset = addr;
    return true;
}

bool read_ov13855_eeprom( kal_uint16 addr, BYTE* data, kal_uint32 size){
	int i;
	addr = 0x0d00;
	//size = 1404;
	//BYTE header[9]= {0};
	//_read_3P3_eeprom(0x0000, header, 9);

	LOG_INF("read 3P3 eeprom, size = %d\n", size);

	if(!get_done || last_size != size || last_offset != addr) {
		LOG_INF("read 3P3 eeprom, size = %d\n", size);
		if(!_read_ov13855_eeprom(addr, ov13855_eeprom_data, size)){
			get_done = 0;
          	  		last_size = 0;
           			 last_offset = 0;
			return false;
		}
	}
//if((ov13855_eeprom_data[0] != 1) || (ov13855_eeprom_data[498] != 1) || (ov13855_eeprom_data[1306] != 1))
	if((ov13855_eeprom_data[0] != 0x40))
	{
		LOG_INF("ov13855 PDAF eeprom data invalid\n");
		return false;
	}
	
	for(i = 0; i < 1372; i++)   // proc1 data
	{
		data[i] = ov13855_eeprom_data[i+1];
	}
	for(i = 0; i < 1372; i++)
	{
		LOG_INF("data[%d] = %d\n", i, data[i]);
	}
	//memcpy(data, ov13855_eeprom_data, size); //huangsh4 make pdaf buffer correct
    return true;
}


