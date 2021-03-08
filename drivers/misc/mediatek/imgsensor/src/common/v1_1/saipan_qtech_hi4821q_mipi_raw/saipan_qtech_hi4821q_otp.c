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

#define LOG_INF(format, args...)    pr_info(PFX "[%s] " format, __func__, ##args)
#define LOG_DBG(format, args...)    pr_info(PFX "[%s] " format, __func__, ##args)
#define LOG_ERR(format, args...)    pr_info(PFX "[%s] " format, __func__, ##args)

u8* bpgc_data_buffer = NULL;
u8* qgc_data_buffer = NULL;
u8* xgc_data_buffer = NULL;
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
//XGC,QGC,BPGC Calibration data are applied here
void apply_sensor_XGC_QGC_Cali(void)
{
	kal_uint16 idx = 0;
	kal_uint16 sensor_qgc_addr;
	kal_uint16 sensor_xgc_addr;

	kal_uint16 hi4821_xgc_data;
	kal_uint16 hi4821_qgc_data;

	int i;
	sensor_xgc_addr = SRAM_XGC_START_ADDR_48M + 2;

	write_cmos_sensor_fun(0x0b00,0x0000);
	write_cmos_sensor_fun(0x0318,0x0001);//for EEPROM
	//isp_reg_en = read_cmos_sensor_fun(0x0b04);
	//write_cmos_sensor_fun(0x0b04,isp_reg_en|0x000E); //XGC, QGC, BPGC enable
	LOG_INF("[Start]:apply_sensor_Cali finish\n");
	//XGC data apply
	{
		write_cmos_sensor_fun(0x301c,0x0002);
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
				LOG_INF("sensor_xgc_addr:0x%x,[ERROR]:no XGC data need apply\n",sensor_xgc_addr);
			}
			idx++;
			write_cmos_sensor_fun(sensor_xgc_addr,hi4821_xgc_data);
			#if SAIPAN_QTECH_HI4821Q_OTP_DUMP
				pr_info("sensor_xgc_addr:0x%x,xgc_data_buffer[%d]:0x%x,xgc_data_buffer[%d]:0x%x,hi4821_xgc_data:0x%x\n",
					sensor_xgc_addr,i,xgc_data_buffer[i],i+1,xgc_data_buffer[i+1],hi4821_xgc_data);
			#endif

			sensor_xgc_addr += 4;
		}
	}

	//QGC data apply
	{
		write_cmos_sensor_fun(0x301c,0x0002);
		sensor_qgc_addr = SRAM_QGC_START_ADDR_48M;
		for(i = 0; i < QGC_BLOCK_X*QGC_BLOCK_Y*16;i += 2) //9(BLOCK_X)* 7 (BLCOK_Y)*16(channel)
		{
			hi4821_qgc_data = ((((qgc_data_buffer[i+1]) & (0x00ff)) << 8) + ((qgc_data_buffer[i]) & (0x00ff)));
			write_cmos_sensor_fun(sensor_qgc_addr,hi4821_qgc_data);

			#if SAIPAN_QTECH_HI4821Q_OTP_DUMP
				pr_info("sensor_qgc_addr:0x%x,qgc_data_buffer[%d]:0x%x,qgc_data_buffer[%d]:0x%x,hi4821_qgc_data:0x%x\n",
					sensor_qgc_addr,i,qgc_data_buffer[i],i+1,qgc_data_buffer[i+1],hi4821_qgc_data);
			#endif
			sensor_qgc_addr += 2;
		}
		mdelay(10);
		LOG_INF("[End]:apply_sensor_Cali QGC_XGC_finish\n");
	}
}

void apply_sensor_BPGC_Cali(void)
{
	kal_uint16 idx = 0;
	kal_uint16 sensor_bpgc_addr;
	kal_uint16 hi4821_bpgc_data;

	int i;
	//int isp_reg_en;
	write_cmos_sensor_fun(0x0b00,0x0000);
	write_cmos_sensor_fun(0x0318,0x0001); // for EEPROM
	LOG_INF("[Start]:apply_sensor_Cali BPGC\n");
	//BPGC data apply
	{
		idx = 0;
		write_cmos_sensor_fun(0x301c,0x0000);
		sensor_bpgc_addr = SRAM_BPGC_START_ADDR_48M;
		for(i = 0; i < BPGC_BLOCK_X*BPGC_BLOCK_Y*4;i += 2) //33(BLOCK_X)* 25(BLCOK_Y)*1(channel)*2bytes
		{
			hi4821_bpgc_data = ((((bpgc_data_buffer[i+1]) & (0x00ff)) << 8) + ((bpgc_data_buffer[i]) & (0x00ff)));
			write_cmos_sensor_fun(sensor_bpgc_addr,hi4821_bpgc_data);

			#if SAIPAN_QTECH_HI4821Q_OTP_DUMP
				pr_info("sensor_bpgc_addr:0x%x,bpgc_data_buffer[%d]:0x%x,bpgc_data_buffer[%d]:0x%x,hi4821_bpgc_data:0x%x\n",
					sensor_bpgc_addr,i,bpgc_data_buffer[i],i+1,bpgc_data_buffer[i+1],hi4821_bpgc_data);
			#endif
			sensor_bpgc_addr += 2;
			idx++;
		}
	}

	//write_cmos_sensor_fun(0x0b00,0x0100);
	mdelay(10);
	//write_cmos_sensor_fun(0x0b00,0x0100);
	LOG_INF("[End]:apply_sensor_Cali BPGC_finish\n");
}
#endif
