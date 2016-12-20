#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/slab.h>


#define PFX "IMX300_pdafotp"
#define LOG_INF(format, args...)	pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)


#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);


#define Sleep(ms) mdelay(ms)

#define IMX300_EEPROM_READ_ID     (0xA0)
#define PAGE_SIZE_IMX300                 (256)
#define IMX300_DCC_DATA_SIZE        (192)
#define IMX300_DCC_DATA_SIZE_MEM        (384)
#define IMX300_DCC_CONVERT_COEFFICIENT  (0x8F5C)

u8 IMX300_DCC_data_3point5[IMX300_DCC_DATA_SIZE]= {0};
u8 IMX300_INFO_data[31]= {0};

u16 IMX300_DCC_data_7point9[IMX300_DCC_DATA_SIZE]= {0};

u8 IMX300_ACT_data[2]= {0};

#if 0
u8 IMX300_SPC_data[352]= {0};
#endif

static bool get_done = false;
static int last_size = 0;
static int last_offset = 0;



static bool selective_read_byte(u32 addr, u8 *data)
{

	u8 page = addr / PAGE_SIZE_IMX300; /* size of page was 256 */
	u8 offset = addr % PAGE_SIZE_IMX300;

	if (iReadRegI2C(&offset, 1, (u8 *)data, 1, IMX300_EEPROM_READ_ID + (page << 1)) < 0) 
	{
		LOG_INF("jeff fail selective_read_byte addr =0x%x data = 0x%x,page %d, offset 0x%x", addr, *data, page, offset);
		return false;
	}
	LOG_INF("jeff selective_read_byte addr =0x%x data = 0x%x,page %d, offset 0x%x", addr,	*data,page,offset); 
	return true;
}

static bool _read_imx300_eeprom(u16 addr, u8* data, int size )
{
	int i = 0;
	int offset = addr;
	LOG_INF("enter _read_eeprom size = %d\n",size);
	for(i = 0; i < size; i++) 
	{
		if(!selective_read_byte(offset, &data[i]))
		{
			return false;
		}
		LOG_INF("read_eeprom 0x%x 0x%x\n",offset, data[i]);
		offset++;
	}
	get_done = true;
	last_size = size;
	last_offset = addr;
    return true;
}

#if 0
void read_imx300_SPC(u8* data)
{
	int addr = 0x2E8;
	int size = 352;
	
	LOG_INF("read imx300 SPC, size = %d\n", size);
	
	if(!get_done || last_size != size || last_offset != addr) {
		if(!_read_imx300_eeprom(addr, IMX300_SPC_data, size)){
			get_done = 0;
            last_size = 0;
            last_offset = 0;
		}
	}

	memcpy(data, IMX300_SPC_data , size);
}
#endif

void read_imx300_DCC(void)
{
	u16 addr = 0x61c;
	u32 size = IMX300_DCC_DATA_SIZE;
	int index;

	LOG_INF("Jeff read imx300 DCC, size = %d\n", size);
	
	if(!get_done || last_size != size || last_offset != addr) 
	{
		if(!_read_imx300_eeprom(addr, IMX300_DCC_data_3point5, size))
		{
			get_done = 0;
			last_size = 0;
			last_offset = 0;
		}
	}
	for (index = 0; index <  IMX300_DCC_DATA_SIZE; index+=16)
	{
		LOG_INF("addr(0x61c + (%x))  %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x \n",
			index, 
			IMX300_DCC_data_3point5[index],
			IMX300_DCC_data_3point5[index + 1],
			IMX300_DCC_data_3point5[index + 2],
			IMX300_DCC_data_3point5[index + 3],

			IMX300_DCC_data_3point5[index + 4],
			IMX300_DCC_data_3point5[index + 5],
			IMX300_DCC_data_3point5[index + 6],
			IMX300_DCC_data_3point5[index + 7],

			IMX300_DCC_data_3point5[index + 8],
			IMX300_DCC_data_3point5[index + 9],
			IMX300_DCC_data_3point5[index + 10],
			IMX300_DCC_data_3point5[index + 11],

			IMX300_DCC_data_3point5[index + 12],
			IMX300_DCC_data_3point5[index + 13],
			IMX300_DCC_data_3point5[index + 14],
			IMX300_DCC_data_3point5[index + 15]);
	}

}


void read_imx300_module_info(void)
{
	u16 addr = 0;
	u32 size = 31;
	int i;

	LOG_INF("Jeff read imx300 info, size = %d\n", size);
	
	for(i = 0; i < size; i++) 
	{
		if(!selective_read_byte(addr, &IMX300_INFO_data[i]))
		{
			LOG_INF("read_eeprom info failed\n");

		}
		LOG_INF("read_eeprom 0x%x  0x%x\n",addr, IMX300_INFO_data[i]);
		addr++;
	}

}

void read_imx300_actuator_sensitivity(u16 *act_sensitivity)
{
	u8 page = 0x8a / PAGE_SIZE_IMX300; /* size of page was 256 */
	u8 offset = 0x8a % PAGE_SIZE_IMX300;

	if (iReadRegI2C(&offset, 1, (u8 *)&IMX300_ACT_data[0], 1, IMX300_EEPROM_READ_ID + (page << 1)) < 0) 
	{
		LOG_INF("jeff fail actuator_sensitivity  data[0]\n");
	}
	offset++;
	if (iReadRegI2C(&offset, 1, (u8 *)&IMX300_ACT_data[1], 1, IMX300_EEPROM_READ_ID + (page << 1)) < 0) 
	{
		LOG_INF("jeff fail actuator_sensitivity data[1] \n");
	}
	LOG_INF("actuator_sensitivity  data[0] (0x%x) data[1] (0x%x)",  IMX300_ACT_data[0], IMX300_ACT_data[1]);
       *act_sensitivity = IMX300_ACT_data[0]<<8 |(IMX300_ACT_data[1] & 0xFF);
}

void imx300_DCC_conversion(u16 addr,u8* data, u32 size)
{
       u16 act_sensitivity;
	int i;
	u64 dataCon;

	read_imx300_DCC();
	read_imx300_actuator_sensitivity(&act_sensitivity);
	LOG_INF("actuator_sensitivity  (0x%x)\n", act_sensitivity);

	for(i = 0; i < IMX300_DCC_DATA_SIZE; i++)
	{ 
	   dataCon = (IMX300_DCC_data_3point5[i] << 8) * (IMX300_DCC_CONVERT_COEFFICIENT) /act_sensitivity;  //Q13
          LOG_INF("dataCon  (%lld)\n", dataCon);

	   IMX300_DCC_data_7point9[i] = dataCon >> 4;  //u7.9 bmj0817
	   LOG_INF("IMX300_DCC_data_7point9[%d]  (%u)\n", i, IMX300_DCC_data_7point9[i]);

	}
	
	read_imx300_module_info();
	memcpy(data, IMX300_DCC_data_7point9 , IMX300_DCC_DATA_SIZE_MEM);
}
