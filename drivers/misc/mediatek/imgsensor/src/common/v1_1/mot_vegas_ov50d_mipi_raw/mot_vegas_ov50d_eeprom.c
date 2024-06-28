#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "imgsensor_ca.h"

#include "mot_vegas_ov50dmipiraw_Sensor.h"

static int mot_sensor_debug = 1;

typedef struct {
	MUINT16 addr;
	MUINT16 data;
} vegas_ov_cal_addr_data_t;

#define PFX "MOT_VEGAS_OV50D"
#define LOG_INF(format, args...)        do { if (mot_sensor_debug   ) { pr_err(PFX "[%s] " format, __func__,##args); } } while(0)
#define LOG_ERR(format, args...)        do { if (mot_sensor_debug   ) { pr_err(PFX "[%s] " format, __func__,##args); } } while(0)
#define LOG_INF_N(format, args...)   pr_warn(PFX "[%s] " format, __func__, ##args)
#define LOG_ERROR(format, args...)   pr_err(PFX "[%s] " format, __func__, ##args)

static DEFINE_SPINLOCK(imgsensor_lock);
static  struct imgsensor_struct *imgsensor;

#define VEGAS_OV50D_EEPROM_SLAVE_ADDR 0xA0
#define VEGAS_OV50D_SENSOR_IIC_SLAVE_ADDR 0x20
#define VEGAS_OV50D_EEPROM_SIZE  0x1CC
#define VEGAS_OV50D_EEPROM_CRC_PDC_SIZE 458
#define VEGAS_OV50D_EEPROM_CRC_PDC_WRITE_SIZE 450

static vegas_ov_cal_addr_data_t ov_pdc_data[VEGAS_OV50D_EEPROM_CRC_PDC_WRITE_SIZE] = {{0}};

int pdc_data_valid = 0;

static uint8_t VEGAS_OV50D_eeprom[VEGAS_OV50D_EEPROM_SIZE] = {0};

extern kal_uint16 mot_vegas_ov50d_burst_write_cmos_sensor(kal_uint16 *para, kal_uint32 len);

static uint8_t crc_reverse_byte(uint32_t data)
{
	return ((data * 0x0802LU & 0x22110LU) |
		(data * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}

static uint32_t convert_crc(uint8_t *crc_ptr)
{
	return (crc_ptr[0] << 8) | (crc_ptr[1]);
}

static int32_t eeprom_util_check_crc16(uint8_t *data, uint32_t size, uint32_t ref_crc)
{
	int32_t crc_match = 0;
	uint16_t crc = 0x0000;
	uint16_t crc_reverse = 0x0000;
	uint32_t i, j;

	uint32_t tmp;
	uint32_t tmp_reverse;

	/* Calculate both methods of CRC since integrators differ on
	* how CRC should be calculated. */
	for (i = 0; i < size; i++) {
		tmp_reverse = crc_reverse_byte(data[i]);
		tmp = data[i] & 0xff;
		for (j = 0; j < 8; j++) {
			if (((crc & 0x8000) >> 8) ^ (tmp & 0x80))
				crc = (crc << 1) ^ 0x8005;
			else
				crc = crc << 1;
			tmp <<= 1;

			if (((crc_reverse & 0x8000) >> 8) ^ (tmp_reverse & 0x80))
				crc_reverse = (crc_reverse << 1) ^ 0x8005;
			else
				crc_reverse = crc_reverse << 1;

			tmp_reverse <<= 1;
		}
	}

	crc_reverse = (crc_reverse_byte(crc_reverse) << 8) |
		crc_reverse_byte(crc_reverse >> 8);

	if (crc == ref_crc || crc_reverse == ref_crc)
		crc_match = 1;

	LOG_INF("REF_CRC 0x%x CALC CRC 0x%x CALC Reverse CRC 0x%x matches? %d\n",
		ref_crc, crc, crc_reverse, crc_match);

	return crc_match;
}

static struct IMGSENSOR_I2C_CFG *get_i2c_cfg(void)
{
	return &(((struct IMGSENSOR_SENSOR_INST *)
		  (imgsensor->psensor_func->psensor_inst))->i2c_cfg);
}

static kal_uint16 VEGAS_OV50D_read_cmos_sensor_8(kal_uint16 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

	imgsensor_i2c_read(
		get_i2c_cfg(),
		pusendcmd,
		2,
		(u8 *)&get_byte,
		1,
		imgsensor->i2c_write_id,
		IMGSENSOR_I2C_SPEED);
	return get_byte;
}

static void VEGAS_OV50D_read_data_from_eeprom(kal_uint8 slave, kal_uint32 start_add, uint32_t size)
{
	int i = 0;
	spin_lock(&imgsensor_lock);
	imgsensor->i2c_write_id = slave;
	spin_unlock(&imgsensor_lock);

	for (i = 0; i < size; i ++) {
		VEGAS_OV50D_eeprom[i] = VEGAS_OV50D_read_cmos_sensor_8(start_add);
		start_add ++;
	}

	spin_lock(&imgsensor_lock);
	imgsensor->i2c_write_id = VEGAS_OV50D_SENSOR_IIC_SLAVE_ADDR;
	spin_unlock(&imgsensor_lock);
}

int get_ov_pdc_data(void *data)
{
	int i;
	struct VEGAS_OV50D_eeprom_t *eeprom = (struct VEGAS_OV50D_eeprom_t*)data;
	if (!eeprom_util_check_crc16(eeprom->ov_pdc_data, VEGAS_OV50D_EEPROM_CRC_PDC_SIZE,
		convert_crc(eeprom->ov_pdc_crc)))
	{
		pr_debug("PDC Data CRC Fail!");
		pdc_data_valid = 0;
	}
	else
	{
		pr_debug("PDC Data CRC Pass");
		pdc_data_valid = 1;
		for (i = 0; i < VEGAS_OV50D_EEPROM_CRC_PDC_WRITE_SIZE; i++)
		{
			ov_pdc_data[i].addr = 0x59F0 + i;
			ov_pdc_data[i].data = eeprom->ov_pdc_data[8 + i];
		}
		pr_debug("X");

	}
	return 1;
}

void VEGAS_OV50D_eeprom_format_calibration_data(struct imgsensor_struct *pImgsensor)
{
	imgsensor = pImgsensor;
	VEGAS_OV50D_read_data_from_eeprom(VEGAS_OV50D_EEPROM_SLAVE_ADDR, 0x19ED, VEGAS_OV50D_EEPROM_SIZE);
	get_ov_pdc_data(VEGAS_OV50D_eeprom);
}

void write_pdc_data(void)
{
	uint16_t write_table[VEGAS_OV50D_EEPROM_CRC_PDC_WRITE_SIZE * 2] = {0};

	pr_debug("E\n");

	memcpy(write_table, &ov_pdc_data[0].addr, sizeof(write_table));

	mot_vegas_ov50d_burst_write_cmos_sensor(write_table,
		sizeof(write_table)/sizeof(uint16_t));

	pr_debug("apply pdc calibration data success.");
	pr_debug("X");
}
