/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 Hi843b_mipi_raw_otp.c
 *
 * Project:
 * --------
 *	 ALPS
 *
 * Description:
 * ------------
 *	 Source code of Sensor driver
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include <linux/delay.h>
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include <linux/types.h>
#include "kd_camera_typedef.h"

#define PFX "HI843B_OTP"
#define LOG_INF(format, args...)	printk(PFX "[%s] " format, __FUNCTION__, ##args)
#define SLAVE_ID (0x40)

//odm.sw START liuzhen config kungfu set OTP typical value for HI843
#define RG_Ratio_Typical (0x156)
#define BG_Ratio_Typical (0x120)
//odm.sw START liuzhen config kungfu set OTP typical value for HI843

extern void kdSetI2CSpeed(u32 i2cSpeed);
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iReadRegI2C_OTP(u8 *a_pSendData , u16 a_sizeSendData, u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;

	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
	kdSetI2CSpeed(400); 
	//iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, SLAVE_ID);
	iReadRegI2C_OTP(pu_send_cmd, 2, (u8*)&get_byte, 1, SLAVE_ID);

	return get_byte;
}

static void HI843B_OTP_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para)};
	kdSetI2CSpeed(400); 
	iWriteRegI2C(pu_send_cmd, 3, SLAVE_ID);

}

void HI843BOTPSetting(void)
{
	LOG_INF("%s enter\n",__func__);
	HI843B_OTP_write_cmos_sensor(0x0A02, 0x01); //Fast sleep on
	HI843B_OTP_write_cmos_sensor(0x0A00, 0x00);//stand by on
	mdelay(10);
	HI843B_OTP_write_cmos_sensor(0x0f02, 0x00);//pll disable
	HI843B_OTP_write_cmos_sensor(0x071a, 0x01);//CP TRIM_H
	HI843B_OTP_write_cmos_sensor(0x071b, 0x09);//IPGM TRIM_H
	HI843B_OTP_write_cmos_sensor(0x0d04, 0x01);//Fsync(OTP busy)Output Enable 
	HI843B_OTP_write_cmos_sensor(0x0d00, 0x07);//Fsync(OTP busy)Output Drivability
	HI843B_OTP_write_cmos_sensor(0x003e, 0x10);//OTP r/w mode
	HI843B_OTP_write_cmos_sensor(0x0a00, 0x01);//standby off

	LOG_INF("%s exit\n",__func__);
}

static kal_uint16 HI843B_Sensor_OTP_read(kal_uint16 otp_addr)
{
	kal_uint16 i, data;
	i = otp_addr;

	HI843B_OTP_write_cmos_sensor(0x070a, (i >> 8) & 0xFF); //start address H
	HI843B_OTP_write_cmos_sensor(0x070b, i & 0xFF); //start address L
	HI843B_OTP_write_cmos_sensor(0x0702, 0x01); //read enable
	data = read_cmos_sensor(0x0708); //OTP data read
	HI843B_OTP_write_cmos_sensor(0x003e, 0x00); //complete
	return data;
}

void Hi843B_Sensor_OTP_info(void)
{
	uint16_t ModuleHouseID = 0,
		 AFFF = 0,
		 Year = 0,
		 Month = 0,
		 Day = 0,
		 SensorID = 0,
		 LensID = 0,
		 VCMID = 0,
		 DriverID = 0,
		 FNoID = 0,
		 iRID = 0;

	uint16_t info_flag = 0,
		 infocheck = 0,
		 checksum = 0;

	LOG_INF("%s enter\n",__func__);
	//info_flag = HI843B_Sensor_OTP_read(0xBCEA);
	if (HI843B_Sensor_OTP_read(0x0201) == 0x01)
	{info_flag = 1;}
	else if (HI843B_Sensor_OTP_read(0x0201) == 0x13)
	{info_flag = 2;}
	else if (HI843B_Sensor_OTP_read(0x0201) == 0x37)
	{info_flag = 3;}
	printk("%s,info_flag = 0x%x\n",__func__,info_flag);
	switch (info_flag)
	{
		case 0x01:  //Group1
			ModuleHouseID = HI843B_Sensor_OTP_read(0x0202);
			AFFF = HI843B_Sensor_OTP_read(0x0203);
			Year = HI843B_Sensor_OTP_read(0x0204);
			Month = HI843B_Sensor_OTP_read(0x0205);
			Day = HI843B_Sensor_OTP_read(0x0206);
			SensorID = HI843B_Sensor_OTP_read(0x0207);
			LensID = HI843B_Sensor_OTP_read(0x0208);
			VCMID = HI843B_Sensor_OTP_read(0x0209);
			DriverID = HI843B_Sensor_OTP_read(0x020a);
			FNoID = HI843B_Sensor_OTP_read(0x020b);
			iRID = HI843B_Sensor_OTP_read(0x020c);
			infocheck = HI843B_Sensor_OTP_read(0x0212);   //020f  20160524
			break;
		case 0x02:
			ModuleHouseID = HI843B_Sensor_OTP_read(0x0213);
			AFFF = HI843B_Sensor_OTP_read(0x0214);
			Year = HI843B_Sensor_OTP_read(0x0215);
			Month = HI843B_Sensor_OTP_read(0x0216);
			Day = HI843B_Sensor_OTP_read(0x0217);
			SensorID = HI843B_Sensor_OTP_read(0x0218);
			LensID = HI843B_Sensor_OTP_read(0x0219);
			VCMID = HI843B_Sensor_OTP_read(0x021a);
			DriverID = HI843B_Sensor_OTP_read(0x021b);
			FNoID = HI843B_Sensor_OTP_read(0x021c);
			iRID = HI843B_Sensor_OTP_read(0x021d);
			infocheck = HI843B_Sensor_OTP_read(0x0223);     //0220  20160524
			break;
		case 0x03:
			ModuleHouseID = HI843B_Sensor_OTP_read(0x0224);
			AFFF = HI843B_Sensor_OTP_read(0x0225);
			Year = HI843B_Sensor_OTP_read(0x0226);
			Month = HI843B_Sensor_OTP_read(0x0227);
			Day = HI843B_Sensor_OTP_read(0x0228);
			SensorID = HI843B_Sensor_OTP_read(0x0229);
			LensID = HI843B_Sensor_OTP_read(0x022a);
			VCMID = HI843B_Sensor_OTP_read(0x022b);
			DriverID = HI843B_Sensor_OTP_read(0x022c);
			FNoID = HI843B_Sensor_OTP_read(0x022d);
			iRID = HI843B_Sensor_OTP_read(0x022e);
			infocheck = HI843B_Sensor_OTP_read(0x0234);     //0231  20160524
			break;
		default:
			LOG_INF("HI843B_Sensor: info_flag error value: %d \n ",info_flag);
			printk("wangk_HI843B_Sensor: info_flag error value: %d \n ",info_flag);
			break;
	}

	checksum = (ModuleHouseID + AFFF + Year + Month + Day + SensorID + LensID + VCMID + DriverID + FNoID + iRID) % 256;

	// checksum = (ModuleHouseID + Year + Month + Day + SensorID + LensID + VCMID + DriverID ) % 256;
	if (checksum == infocheck)
	{
		LOG_INF("HI843B_Sensor: Module information checksum PASS\n ");
		printk("wangk_HI843B_Sensor: Module information checksum PASS\n ");
	}
	else
	{
		LOG_INF("HI843B_Sensor: Module information checksum Fail\n ");
		printk("wangk_HI843B_Sensor: Module information checksum Fail\n ");
	}

	LOG_INF("ModuleHouseID = %d \n", ModuleHouseID);
	LOG_INF("AFFF = %d \n", AFFF);
	LOG_INF("Year = %d, Month = %d, Day = %d\n", Year, Month, Day);
	LOG_INF("SensorID = %d \n", SensorID);
	LOG_INF("LensID = %d \n", LensID);
	LOG_INF("VCMID = %d \n", VCMID);
	LOG_INF("DriverID = %d \n", DriverID);
	LOG_INF("FNoID = %d \n", FNoID);
	LOG_INF("checksum = %d \n", checksum);


	LOG_INF("%s exit\n",__func__);

}

void Hi843B_Sensor_OTP_update_LSC(void)
{
	uint16_t lsc_flag = 0,
		 temp = 0;
	LOG_INF("%s enter\n",__func__);
	//lsc_flag = HI843B_Sensor_OTP_read(0xBCED);
	if (HI843B_Sensor_OTP_read(0x0235) == 0x01)
	{lsc_flag = 1;}
	else if (HI843B_Sensor_OTP_read(0x0235) == 0x13)
	{lsc_flag = 2;}
	else if (HI843B_Sensor_OTP_read(0x0235) == 0x37)
	{lsc_flag = 3;}
	printk("%s,lsc_flag = 0x%x\n",__func__,lsc_flag);

	temp = read_cmos_sensor (0x0a05) | 0x14;	//bit[4] = 1, lsc enable
	//`temp = temp & 0xF7;
	printk("%s,lsc_enable flag = 0x%x\n",__func__,temp);
	HI843B_OTP_write_cmos_sensor(0x0a05, temp); //LSC enable
	LOG_INF("%s exit\n",__func__);
}

static void HI843B_Sensor_update_wb_gain(kal_uint32 r_gain, kal_uint32 g_gain,kal_uint32 b_gain)
{
	kal_int16 temp;

	printk("%s,r_gain = 0x%x, b_gain = 0x%x\n", __func__,r_gain, b_gain);

	HI843B_OTP_write_cmos_sensor(0x0508, g_gain >> 8); //gr_gain
	HI843B_OTP_write_cmos_sensor(0x0509, g_gain & 0xFF); //gr_gain
	HI843B_OTP_write_cmos_sensor(0x050a, g_gain >> 8); //gb_gain
	HI843B_OTP_write_cmos_sensor(0x050b, g_gain & 0xFF); //gb_gain
	HI843B_OTP_write_cmos_sensor(0x050c, r_gain >> 8); //r_gain
	HI843B_OTP_write_cmos_sensor(0x050d, r_gain & 0xFF); //r_gain
	HI843B_OTP_write_cmos_sensor(0x050e, b_gain >> 8); //b_gain
	HI843B_OTP_write_cmos_sensor(0x050f, b_gain & 0xFF); //b_gain

	temp = read_cmos_sensor(0x0a05) | 0x08;
	printk("%s,0x0a05 = 0x%x\n", __func__,temp);
	HI843B_OTP_write_cmos_sensor(0x0a05, temp); //Digital Gain enable
}

void HI843B_Sensor_calc_wbdata(void)
{
	uint16_t wbcheck = 0,
		 checksum = 0,
		 wb_flag = 0;
	uint16_t r_gain = 0x200,
		 b_gain = 0x200,
		 g_gain = 0x200;
	volatile uint16_t wb_unit_rg_h = 0,
		 wb_unit_rg_l = 0,
		 wb_unit_bg_h = 0,
		 wb_unit_bg_l = 0,
		 wb_unit_gg_h = 0,
		 wb_unit_gg_l = 0,
		 wb_golden_rg_h = 0,
		 wb_golden_rg_l = 0,
		 wb_golden_bg_h = 0,
		 wb_golden_bg_l = 0,
		 wb_golden_gg_h = 0,
		 wb_golden_gg_l = 0;
#if 0 //pangfei del
	uint16_t wb_unit_r_h = 0,
		 wb_unit_r_l = 0,
		 wb_unit_b_h = 0,
		 wb_unit_b_l = 0,
		 wb_unit_g_h = 0,
		 wb_unit_g_l = 0,

		 wb_golden_r_h = 0,
		 wb_golden_r_l = 0,
		 wb_golden_b_h = 0,
		 wb_golden_b_l = 0,
		 wb_golden_g_h = 0,
		 wb_golden_g_l = 0;
#endif

	uint16_t rg_golden_value = 0,
		 bg_golden_value = 0,
		 rg_value = 0,
		 bg_value = 0;
	//uint16_t temp_rg,temp_bg;

	LOG_INF("%s enter\n",__func__);
	wb_flag = HI843B_Sensor_OTP_read(0x0c5f);
	printk("wb_flag = 0x%x %s\n",wb_flag,__func__);
	if (HI843B_Sensor_OTP_read(0x0c5f) == 0x01)
	{
		wb_flag = 1;
	}
	else if (HI843B_Sensor_OTP_read(0x0c5f) == 0x13)
	{
		wb_flag = 2;
	}
	else if (HI843B_Sensor_OTP_read(0x0c5f) == 0x37)
	{
		wb_flag = 3;
	}
	printk("otp wb valid group = 0x%x %s\n",wb_flag,__func__);//check flag
	switch (wb_flag)
	{
		case 0x01:  //Group1 valid
			wb_unit_rg_h = HI843B_Sensor_OTP_read(0x0c60);
			wb_unit_rg_l = HI843B_Sensor_OTP_read(0x0c61);
			wb_unit_bg_h = HI843B_Sensor_OTP_read(0x0c62);
			wb_unit_bg_l = HI843B_Sensor_OTP_read(0x0c63);
			wb_unit_gg_h = HI843B_Sensor_OTP_read(0x0c64);
			wb_unit_gg_l = HI843B_Sensor_OTP_read(0x0c65);
			wb_golden_rg_h = HI843B_Sensor_OTP_read(0x0c66);
			wb_golden_rg_l = HI843B_Sensor_OTP_read(0x0c67);
			wb_golden_bg_h = HI843B_Sensor_OTP_read(0x0c68);
			wb_golden_bg_l = HI843B_Sensor_OTP_read(0x0c69);
			wb_golden_gg_h = HI843B_Sensor_OTP_read(0x0c6a);
			wb_golden_gg_l = HI843B_Sensor_OTP_read(0x0c6b);
			// wb_unit_r_h = HI843B_Sensor_OTP_read(0x0c6c);
			// wb_unit_r_l = HI843B_Sensor_OTP_read(0x0c6d);
			// wb_unit_b_h = HI843B_Sensor_OTP_read(0x0c6e);
			// wb_unit_b_l = HI843B_Sensor_OTP_read(0x0c6f);
			// wb_unit_g_h = HI843B_Sensor_OTP_read(0x0c70);
			// wb_unit_g_l = HI843B_Sensor_OTP_read(0x0c71);
			// wb_golden_r_h = HI843B_Sensor_OTP_read(0x0c72);
			// wb_golden_r_l = HI843B_Sensor_OTP_read(0x0c73);
			// wb_golden_b_h = HI843B_Sensor_OTP_read(0x0c74);
			// wb_golden_b_l = HI843B_Sensor_OTP_read(0x0c75);
			// wb_golden_g_h = HI843B_Sensor_OTP_read(0x0c76);
			// wb_golden_g_l = HI843B_Sensor_OTP_read(0x0c77);
			wbcheck = HI843B_Sensor_OTP_read(0x0c6f);
			break;

		case 0x02:  //Group2 valid
			wb_unit_rg_h = HI843B_Sensor_OTP_read(0x0c70);
			wb_unit_rg_l = HI843B_Sensor_OTP_read(0x0c71);
			wb_unit_bg_h = HI843B_Sensor_OTP_read(0x0c72);
			wb_unit_bg_l = HI843B_Sensor_OTP_read(0x0c73);
			wb_unit_gg_h = HI843B_Sensor_OTP_read(0x0c74);
			wb_unit_gg_l = HI843B_Sensor_OTP_read(0x0c75);
			wb_golden_rg_h = HI843B_Sensor_OTP_read(0x0c76);
			wb_golden_rg_l = HI843B_Sensor_OTP_read(0x0c77);
			wb_golden_bg_h = HI843B_Sensor_OTP_read(0x0c78);
			wb_golden_bg_l = HI843B_Sensor_OTP_read(0x0c79);
			wb_golden_gg_h = HI843B_Sensor_OTP_read(0x0c7a);
			wb_golden_gg_l = HI843B_Sensor_OTP_read(0x0c7b);
			// wb_unit_r_h = HI843B_Sensor_OTP_read(0xBD44);
			// wb_unit_r_l = HI843B_Sensor_OTP_read(0xBD45);
			// wb_unit_b_h = HI843B_Sensor_OTP_read(0xBD46);
			// wb_unit_b_l = HI843B_Sensor_OTP_read(0xBD47);
			// wb_unit_g_h = HI843B_Sensor_OTP_read(0xBD48);
			// wb_unit_g_l = HI843B_Sensor_OTP_read(0xBD49);
			// wb_golden_r_h = HI843B_Sensor_OTP_read(0xBD4C);
			// wb_golden_r_l = HI843B_Sensor_OTP_read(0xBD4D);
			// wb_golden_b_h = HI843B_Sensor_OTP_read(0xBD4E);
			// wb_golden_b_l = HI843B_Sensor_OTP_read(0xBD4F);
			// wb_golden_g_h = HI843B_Sensor_OTP_read(0xBD50);
			// wb_golden_g_l = HI843B_Sensor_OTP_read(0xBD51);
			wbcheck = HI843B_Sensor_OTP_read(0x0c7f);
			break;
		case 0x03:  //Group3 valid
			wb_unit_rg_h = HI843B_Sensor_OTP_read(0x0c80);
			wb_unit_rg_l = HI843B_Sensor_OTP_read(0x0c81);
			wb_unit_bg_h = HI843B_Sensor_OTP_read(0x0c82);
			wb_unit_bg_l = HI843B_Sensor_OTP_read(0x0c83);
			wb_unit_gg_h = HI843B_Sensor_OTP_read(0x0c84);
			wb_unit_gg_l = HI843B_Sensor_OTP_read(0x0c85);
			wb_golden_rg_h = HI843B_Sensor_OTP_read(0x0c86);
			wb_golden_rg_l = HI843B_Sensor_OTP_read(0x0c87);
			wb_golden_bg_h = HI843B_Sensor_OTP_read(0x0c88);
			wb_golden_bg_l = HI843B_Sensor_OTP_read(0x0c89);
			wb_golden_gg_h = HI843B_Sensor_OTP_read(0x0c8a);
			wb_golden_gg_l = HI843B_Sensor_OTP_read(0x0c8b);
			//  wb_unit_r_h = HI843B_Sensor_OTP_read(0xBD7D);
			//  wb_unit_r_l = HI843B_Sensor_OTP_read(0xBD7E);
			//  wb_unit_b_h = HI843B_Sensor_OTP_read(0xBD7F);
			//  wb_unit_b_l = HI843B_Sensor_OTP_read(0xBD80);
			//  wb_unit_g_h = HI843B_Sensor_OTP_read(0xBD81);
			//  wb_unit_g_l = HI843B_Sensor_OTP_read(0xBD82);
			//  wb_golden_r_h = HI843B_Sensor_OTP_read(0xBD85);
			//  wb_golden_r_l = HI843B_Sensor_OTP_read(0xBD86);
			//  wb_golden_b_h = HI843B_Sensor_OTP_read(0xBD87);
			//  wb_golden_b_l = HI843B_Sensor_OTP_read(0xBD88);
			//  wb_golden_g_h = HI843B_Sensor_OTP_read(0xBD89);
			//  wb_golden_g_l = HI843B_Sensor_OTP_read(0xBD8A);
			wbcheck = HI843B_Sensor_OTP_read(0x0c8f);
			break;
		default:
			LOG_INF("HI843B_Sensor: wb_flag error value: 0x%x\n ",wb_flag);
			break;
	}
	LOG_INF("HI843B_Sensor: group 1 addr 0x0c60=0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n ",wb_unit_rg_h,wb_unit_rg_l,wb_unit_bg_h,wb_unit_bg_l,wb_unit_gg_h,wb_unit_gg_l,wb_golden_rg_h,wb_golden_rg_l,wb_golden_bg_h,wb_golden_bg_l,wb_golden_gg_h,wb_golden_gg_l);	

	//   checksum = (wb_unit_rg_h + wb_unit_rg_l + wb_unit_bg_h + wb_unit_bg_l + wb_unit_gg_h + wb_unit_gg_l
	//     + wb_golden_rg_h + wb_golden_rg_l + wb_golden_bg_h + wb_golden_bg_l + wb_golden_gg_h + wb_golden_gg_l
	//     + wb_unit_r_h + wb_unit_r_l + wb_unit_b_h + wb_unit_b_l + wb_unit_g_h + wb_unit_g_l
	//     + wb_golden_r_h + wb_golden_r_l + wb_golden_b_h + wb_golden_b_l + wb_golden_g_h + wb_golden_g_l ) % 0xFF + 1;

	checksum = (wb_unit_rg_h + wb_unit_rg_l + wb_unit_bg_h + wb_unit_bg_l + wb_unit_gg_h + wb_unit_gg_l
			+ wb_golden_rg_h + wb_golden_rg_l + wb_golden_bg_h + wb_golden_bg_l + wb_golden_gg_h + wb_golden_gg_l) % 256;


	if (checksum == wbcheck)
	{
		LOG_INF("HI843B_Sensor: WB checksum PASS\n ");
	}
	else
	{
		LOG_INF("HI843B_Sensor: WB checksum Fail\n ");
	}

	//r_gain = ((wb_golden_r_h << 8)|wb_golden_r_l);
	//b_gain = ((wb_golden_b_h << 8)|wb_golden_b_l);
	//g_gain = ((wb_golden_g_h << 8)|wb_golden_g_l);

	//printk("%s,default data: r_gain = 0x%x, g_gain = 0x%x,b_gain = 0x%x\n",__func__, r_gain,g_gain,b_gain);

	//rg_golden_value = ((wb_golden_rg_h << 8)|wb_golden_rg_l);//use otp golden value
	//bg_golden_value = ((wb_golden_bg_h << 8)|wb_golden_bg_l);
	//odm.sw START liuzhen config kungfu set OTP typical value
	rg_golden_value = RG_Ratio_Typical;
	bg_golden_value = BG_Ratio_Typical;
	//odm.sw END liuzhen config kungfu set OTP typical value

	rg_value = ((wb_unit_rg_h << 8)|wb_unit_rg_l);
	bg_value = ((wb_unit_bg_h << 8)|wb_unit_bg_l);

	printk("%s,Read from module: rg_golden_typical = 0x%x, bg_golden_typical = 0x%x\n",__func__, rg_golden_value, bg_golden_value);
	printk("%s,Read from module: rg_value = 0x%x, bg_value = 0x%x\n",__func__, rg_value, bg_value);

#if 1
#define OTP_MULTIPLE_FAC    (128L)   // 128 = 2^7
	r_gain = (((OTP_MULTIPLE_FAC * rg_golden_value) / rg_value) * 0x200) / OTP_MULTIPLE_FAC;
	b_gain = (((OTP_MULTIPLE_FAC * bg_golden_value) / bg_value) * 0x200) / OTP_MULTIPLE_FAC;
#else
	r_gain = (int)(((float)rg_golden_value/rg_value) * 0x200);
	b_gain = (int)(((float)bg_golden_value/bg_value) * 0x200);
#endif

	if(r_gain < b_gain)
	{
		if(r_gain < 0x200)
		{
			b_gain = 0x200 * b_gain/r_gain;
			g_gain = 0x200 * g_gain/r_gain;
			r_gain = 0x200;
		}
	}
	else
	{
		if(b_gain < 0x200)
		{
			r_gain = 0x200 * r_gain/b_gain;
			g_gain = 0x200 * g_gain/b_gain;
			b_gain = 0x200;
		}
	}
	printk("%s,After calcuate: r_gain = 0x%x, g_gain = 0x%x, b_gain = 0x%x\n",__func__, r_gain, g_gain, b_gain);
	HI843B_Sensor_update_wb_gain(r_gain, g_gain,b_gain);
	LOG_INF("%s exit\n",__func__);
}

