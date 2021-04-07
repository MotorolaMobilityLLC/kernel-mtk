#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/types.h>

//#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_typedef.h"
#include "saipan_qtech_hi4821q_otp.h"

#define PFX "HI4821Q_otp"

#define LOG_INF(format, args...)    pr_debug(PFX "[%s] " format, __func__, ##args)
#define LOG_DBG(format, args...)    pr_debug(PFX "[%s] " format, __func__, ##args)
#define LOG_ERR(format, args...)    pr_debug(PFX "[%s] " format, __func__, ##args)

extern kal_uint16 read_cmos_sensor_fun(kal_uint32 addr);
extern void write_cmos_sensor_fun(kal_uint32 addr, kal_uint32 para);

#if SAIPAN_QTECH_HI4821Q_OTP_ENABLE
kal_uint16 saipan_qtech_hi4821q_read_eeprom(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
	char pu_send_cmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };
	iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, EEPROM_BL24SA64D_ID);
	return get_byte;
}
#endif

#if SAIPAN_QTECH_HI4821Q_XGC_QGC_BPGC_CALIB
kal_uint16 saipan_qtech_hi4821q_XGC_setting_burst[XGC_DATA_SIZE] = {0};
kal_uint16 saipan_qtech_hi4821q_QGC_setting_burst[QGC_DATA_SIZE] = {0};
kal_uint16 saipan_qtech_hi4821q_BPGC_setting_burst[BPGC_DATA_SIZE] = {0};
void sensor_format_XGC_QGC_Cali_data( u8* xgc_data_buffer , u8* qgc_data_buffer )
{
	kal_uint16 idx = 0;
	kal_uint16 length1 = 0,length2 = 0;
	kal_uint16 sensor_xgc_addr = 0;
	kal_uint16 sensor_qgc_addr = 0;
	kal_uint16 hi4821_xgc_data = 0;
	kal_uint16 hi4821_qgc_data = 0;
	int i = 0;
	if (NULL == xgc_data_buffer )
		return;
	sensor_xgc_addr = SRAM_XGC_START_ADDR_48M + 2;

	//XGC data format
	for(i = 0; i < XGC_BLOCK_X*XGC_BLOCK_Y*10; i += 2) //9(BLOCK_X)* 7 (BLCOK_Y)*10(channel)
	{
		hi4821_xgc_data = ((((xgc_data_buffer[i+1]) & (0x00ff)) << 8) + ((xgc_data_buffer[i]) & (0x00ff)));
		if(idx == XGC_BLOCK_X*XGC_BLOCK_Y){
			sensor_xgc_addr = SRAM_XGC_START_ADDR_48M;
		}

		else if(idx == XGC_BLOCK_X*XGC_BLOCK_Y*2){
			sensor_xgc_addr += 2;
		}

		else if(idx == XGC_BLOCK_X*XGC_BLOCK_Y*3){
			sensor_xgc_addr = SRAM_XGC_START_ADDR_48M + XGC_BLOCK_X * XGC_BLOCK_Y * 4;
		}

		else if(idx == XGC_BLOCK_X*XGC_BLOCK_Y*4){
			sensor_xgc_addr += 2;
		}

		else{
			//LOG_INF("sensor_xgc_addr:0x%x,[ERROR]:no XGC data need apply\n",sensor_xgc_addr);
		}
		idx++;
		saipan_qtech_hi4821q_XGC_setting_burst[2*length1]=sensor_xgc_addr;
		saipan_qtech_hi4821q_XGC_setting_burst[2*length1+1]=hi4821_xgc_data;
		length1++;
		sensor_xgc_addr += 4;
	}
	if (NULL == qgc_data_buffer )
		return;
	//QGC data format
	sensor_qgc_addr = SRAM_QGC_START_ADDR_48M;
	for(i = 0; i < QGC_BLOCK_X*QGC_BLOCK_Y*16;i += 2) //9(BLOCK_X)* 7 (BLCOK_Y)*16(channel)
	{
		hi4821_qgc_data = ((((qgc_data_buffer[i+1]) & (0x00ff)) << 8) + ((qgc_data_buffer[i]) & (0x00ff)));
		saipan_qtech_hi4821q_QGC_setting_burst[2*length2]=sensor_qgc_addr;
		saipan_qtech_hi4821q_QGC_setting_burst[2*length2+1]=hi4821_qgc_data;
		length2++;
		sensor_qgc_addr += 2;
	}
}

void sensor_format_BPGC_Cali_data(u8* bpgc_data_buffer)
{
	kal_uint16 sensor_bpgc_addr = 0;
	kal_uint16 hi4821_bpgc_data = 0;
	kal_uint16 length3 = 0;
	int i = 0;
	if (NULL == bpgc_data_buffer )
		return;
	sensor_bpgc_addr = SRAM_BPGC_START_ADDR_48M;
	for(i = 0; i < BPGC_BLOCK_X*BPGC_BLOCK_Y*4;i+=2)//13(BLOCK_X)*11(BLCOK_Y)*2(channel)*2bytes
	{
		hi4821_bpgc_data = ((((bpgc_data_buffer[i+1]) & (0x00ff)) <<8) + ((bpgc_data_buffer[i]) & (0x00ff)));
		saipan_qtech_hi4821q_BPGC_setting_burst[2*length3]=sensor_bpgc_addr;
		saipan_qtech_hi4821q_BPGC_setting_burst[2*length3+1]=hi4821_bpgc_data;
		length3++;
		sensor_bpgc_addr += 2;
	}
}
#endif
