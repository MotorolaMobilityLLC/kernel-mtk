#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/slab.h>
#include <linux/namei.h>
/****************************Modify Following Strings for Debug****************************/
#define PFX "S5K3L6mipiraw_pdaf.c"

#ifdef CONFIG_ONTIM_CAMERA_LOG_ON
#define LOG_INF(fmt, args...)   pr_err(PFX "[%s](%d) " fmt, __FUNCTION__,__LINE__, ##args)
#else
#define LOG_INF(fmt, args...)
#endif
/****************************   Modify end    *******************************************/

#include "kd_camera_typedef.h"
//#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
extern void kdSetI2CSpeed(u16 i2cSpeed);
//extern int iBurstWriteReg_multi(u8 *pData, u32 bytes, u16 i2cId, u16 transfer_length);
extern int iMultiReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId, u8 number);
#define write_eeprom_i2c(addr, para) iWriteReg((u16) addr , (u32) para , 1,  EEPROM_READ_ID)

#define USHORT             unsigned short
#define BYTE               unsigned char
#define Sleep(ms)          mdelay(ms)

#define EEPROM_WRITE_ID  0xA0
#define EEPROM_READ_ID   0xA1
#define I2C_SPEED        100  
#define MAX_OFFSET	 0xFFFF

#define START_ADDR_PROC1        0X0765
#define START_ADDR_PROC2        0X0957
#define DATA_SIZE_PROC1         496
#define DATA_SIZE      		1404

BYTE S5K3L6_eeprom_data[DATA_SIZE]= {0};

/* static bool write_s5k3l6_eeprom(kal_uint16 addr, kal_uint16 size, BYTE* data)
{
	int i = 0;
	int count = 0;
	int offset = addr;

	for(i = 0; i < size; i++) {
		write_eeprom_i2c((u16)offset, (u32)*(data + i));
		msleep(2);
		count++;
		offset++;
	}
	if (count == size)
		return true;

	LOG_INF("Eeprom write failed");
	return false;
} 
 */
/**************  CONFIG BY SENSOR <<< ************/

static kal_uint16 read_cmos_sensor_byte(kal_uint16 addr)
{
    kal_uint16 get_byte=0;
    char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };

    kdSetI2CSpeed(I2C_SPEED); // Add this func to set i2c speed by each sensor
    iReadRegI2C(pu_send_cmd , 2, (u8*)&get_byte,1,EEPROM_WRITE_ID);
    return get_byte;
}


static bool _read_eeprom(kal_uint16 addr, kal_uint32 size ){
	//continue read reg by byte:
	static int i = 0;

	printk( "%s(%d)  begin i=%d addr=%d size=%d byyzm \n",__FUNCTION__,__LINE__,i,addr,size);
	for(; i<size; i++){
		S5K3L6_eeprom_data[i] = read_cmos_sensor_byte(addr+i);
		LOG_INF("add = 0x%x,\tvalue = 0x%x",i, S5K3L6_eeprom_data[i]);
	}
	printk( "%s(%d)  return true i=%d addr=%d size=%d byyzm \n",__FUNCTION__,__LINE__,i,addr,size);
	if(i == DATA_SIZE )
	{
		i = 0;
	}	
	return true;
}

bool S5K3L6_read_eeprom( kal_uint16 addr, BYTE* data, kal_uint32 size){
	
	printk( "%s(%d)  begin byyzm \n",__FUNCTION__,__LINE__);
	LOG_INF("Read EEPROM, addr = 0x%x, size = 0d%d\n", addr, size);
/*	
	addr = START_ADDR_PROC1;
	size = DATA_SIZE_PROC1;
	if(!_read_eeprom(addr, size)){
		LOG_INF("error:read_eeprom fail!\n");
		printk( "%s(%d)  return false byyzm \n",__FUNCTION__,__LINE__);
		return false;
	}
*/	
	addr = START_ADDR_PROC1+2;
	size = DATA_SIZE;
	if(!_read_eeprom(addr, size)){
		LOG_INF("error:read_eeprom fail!\n");
		printk( "%s(%d)  return false byyzm \n",__FUNCTION__,__LINE__);
		return false;
	}	
	addr = START_ADDR_PROC1;
	size = DATA_SIZE_PROC1;
	if(!_read_eeprom(addr, size)){
		LOG_INF("error:read_eeprom fail!\n");
		printk( "%s(%d)  return false byyzm \n",__FUNCTION__,__LINE__);
		return false;
	}
	memcpy(data, S5K3L6_eeprom_data, size);
	
	printk( "%s(%d)  return true byyzm \n",__FUNCTION__,__LINE__);
	return true;
}

