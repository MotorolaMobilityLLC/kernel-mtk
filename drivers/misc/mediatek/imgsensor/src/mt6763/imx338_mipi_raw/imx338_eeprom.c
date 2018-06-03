#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/slab.h>
#include "kd_camera_typedef.h"

#define PFX "IMX338_pdafotp"
#define LOG_INF(format, args...)	pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iReadRegI2CTiming(u8 *a_pSendData , u16 a_sizeSendData, u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId, u16 timing);

extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern int iMultiReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId, u8 number);


#define USHORT             unsigned short
#define BYTE               unsigned char
#define Sleep(ms) mdelay(ms)

#define IMX338_EEPROM_READ_ID 0xA0
#define IMX338_EEPROM_WRITE_ID 0xA1
#define IMX338_I2C_SPEED 100
#define IMX338_MAX_OFFSET 4096

#define DATA_SIZE 2048
#define SPC_START_ADDR 0x763
#define DCC_START_ADDR 0x8c3



BYTE IMX338_DCC_data[384]= {0}; // 16x12x2


static bool get_done_dcc = false;
static int last_size_dcc = 0;
static int last_offset_dcc = 0;

static bool get_done_spc = false;
static int last_size_spc = 0;
static int last_offset_spc = 0;


static bool selective_read_eeprom(kal_uint16 addr, BYTE* data)
{
	char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
    if(addr > IMX338_MAX_OFFSET)
        return false;
	if(iReadRegI2C(pu_send_cmd, 2, (u8*)data, 1, IMX338_EEPROM_READ_ID)<0)
		return false;
    return true;
}

static bool _read_imx338_eeprom(kal_uint16 addr, BYTE* data, int size )
{

	int i = 0;
	int offset = addr;
	LOG_INF("enter _read_eeprom size = %d\n",size);
	for(i = 0; i < size; i++) {
		if(!selective_read_eeprom(offset, &data[i])){
			return false;
		}
		/* LOG_INF("read_eeprom 0x%0x %d\n",offset, data[i]); */
		offset++;
	}

	if(addr == SPC_START_ADDR) {
		get_done_spc = true;
		last_size_spc = size;
		last_offset_spc = offset;
	} else {
		get_done_dcc = true;
		last_size_dcc = size;
		last_offset_dcc = offset;
	}
    return true;
}


void read_imx338_SPC(BYTE* data){

	int addr = SPC_START_ADDR;
	int size = 352;
	if(!get_done_spc || last_size_spc != size) {
		if(!_read_imx338_eeprom(addr, data, size)){
			get_done_spc = 0;
			last_size_spc = 0;
			last_offset_spc = 0;
			//return false;
		}
	}

	//memcpy(data, IMX338_SPC_data , size);
    //return true;
}


void read_imx338_DCC( kal_uint16 addr, BYTE* data, kal_uint32 size){
	//int i;
	addr = DCC_START_ADDR;
	size = 384;
	if(!get_done_dcc || last_size_dcc != size ) {
		if(!_read_imx338_eeprom(addr, IMX338_DCC_data, size)){
			get_done_dcc = 0;
			last_size_dcc = 0;
			last_offset_dcc = 0;
			//return false;
		}
	}

	memcpy(data, IMX338_DCC_data , size);
    //return true;
}


