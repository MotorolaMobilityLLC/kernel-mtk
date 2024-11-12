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

#include "mot_bogota_gc32e1mipi_Sensor.h"

static int mot_sensor_debug = 1;

typedef struct {
	MUINT16 addr;
	MUINT16 data;
} kansas_xtc_cal_addr_data_t;

#define PFX "MOT_BOGOTA_GC32E1"
#define LOG_INF(format, args...)        do { if (mot_sensor_debug   ) { pr_err(PFX "[%s] " format, __func__,##args); } } while(0)
#define LOG_ERR(format, args...)        do { if (mot_sensor_debug   ) { pr_err(PFX "[%s] " format, __func__,##args); } } while(0)
#define LOG_INF_N(format, args...)   pr_warn(PFX "[%s] " format, __func__, ##args)
#define LOG_ERROR(format, args...)   pr_err(PFX "[%s] " format, __func__, ##args)

static DEFINE_SPINLOCK(imgsensor_lock);
static  struct imgsensor_struct *imgsensor;

#define BOGOTA_GC32E1_EEPROM_SLAVE_ADDR 0xA2
#define BOGOTA_GC32E1_SENSOR_IIC_SLAVE_ADDR 0x94
#define BOGOTA_GC32E1_EEPROM_SIZE  0x0027
#define BOGOTA_GC32E1_EEPROM_CRC_MANUFACTURING_SIZE 37

static uint8_t BOGOTA_GC32E1_eeprom[BOGOTA_GC32E1_EEPROM_SIZE] = {0};
static mot_calibration_status_t calibration_status = {CRC_FAILURE};
static mot_calibration_mnf_t mnf_info = {0};


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

static kal_uint16 BOGOTA_GC32E1_read_cmos_sensor_8(kal_uint16 addr)
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

static void BOGOTA_GC32E1_read_data_from_eeprom(kal_uint8 slave, kal_uint32 start_add, uint32_t size)
{
	int i = 0;
	spin_lock(&imgsensor_lock);
	imgsensor->i2c_write_id = slave;
	spin_unlock(&imgsensor_lock);

	for (i = 0; i < size; i ++) {
		BOGOTA_GC32E1_eeprom[i] = BOGOTA_GC32E1_read_cmos_sensor_8(start_add);
		start_add ++;
	}

	spin_lock(&imgsensor_lock);
	imgsensor->i2c_write_id = BOGOTA_GC32E1_SENSOR_IIC_SLAVE_ADDR;
	spin_unlock(&imgsensor_lock);
}


static calibration_status_t BOGOTA_GC32E1_check_manufacturing_data(void *data)
{
	struct BOGOTA_GC32E1_eeprom_t *eeprom = (struct BOGOTA_GC32E1_eeprom_t*)data;
	LOG_INF("Manufacturing eeprom->mpn = %.8s !",eeprom->mpn);
	if (!eeprom_util_check_crc16(eeprom->eeprom_table_version, BOGOTA_GC32E1_EEPROM_CRC_MANUFACTURING_SIZE,
		convert_crc(eeprom->manufacture_crc16))) {
		LOG_ERROR("Manufacturing CRC Fails!");
		return CRC_FAILURE;
	}
	LOG_INF("Manufacturing CRC Pass");
	return NO_ERRORS;
}

static void BOGOTA_GC32E1_eeprom_get_mnf_data(void *data,
		mot_calibration_mnf_t *mnf)
{
	int ret;
	struct BOGOTA_GC32E1_eeprom_t *eeprom = (struct BOGOTA_GC32E1_eeprom_t*)data;

	ret = snprintf(mnf->table_revision, MAX_CALIBRATION_STRING, "0x%x",
		eeprom->eeprom_table_version[0]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_ERROR("snprintf of mnf->table_revision failed");
		mnf->table_revision[0] = 0;
	}

	ret = snprintf(mnf->mot_part_number, MAX_CALIBRATION_STRING, "%c%c%c%c%c%c%c%c",
		eeprom->mpn[0], eeprom->mpn[1], eeprom->mpn[2], eeprom->mpn[3],
		eeprom->mpn[4], eeprom->mpn[5], eeprom->mpn[6], eeprom->mpn[7]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_ERROR("snprintf of mnf->mot_part_number failed");
		mnf->mot_part_number[0] = 0;
	}

	ret = snprintf(mnf->actuator_id, MAX_CALIBRATION_STRING, "0x%x", eeprom->actuator_id[0]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_ERROR("snprintf of mnf->actuator_id failed");
		mnf->actuator_id[0] = 0;
	}

	if (eeprom->lens_id[0] == 0xA3){
		ret = snprintf(mnf->lens_id, MAX_CALIBRATION_STRING, "39807A-400");
	} else {
		ret = snprintf(mnf->lens_id, MAX_CALIBRATION_STRING, "Unknown");
		LOG_INF("unknown lens_id");
	}

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_ERROR("snprintf of mnf->lens_id failed");
		mnf->lens_id[0] = 0;
	}

	if (eeprom->manufacturer_id[0] == 'Q' && eeprom->manufacturer_id[1] == 'T') {
		ret = snprintf(mnf->integrator, MAX_CALIBRATION_STRING, "Qtech");
	} else if (eeprom->manufacturer_id[0] == 'T' && eeprom->manufacturer_id[1] == 'S') {
		ret = snprintf(mnf->integrator, MAX_CALIBRATION_STRING, "TSP");
	} else {
		ret = snprintf(mnf->integrator, MAX_CALIBRATION_STRING, "Unknown");
		LOG_INF("unknown manufacturer_id");
	}

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_ERROR("snprintf of mnf->integrator failed");
		mnf->integrator[0] = 0;
	}

	ret = snprintf(mnf->factory_id, MAX_CALIBRATION_STRING, "%c%c",
		eeprom->factory_id[0], eeprom->factory_id[1]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_ERROR("snprintf of mnf->factory_id failed");
		mnf->factory_id[0] = 0;
	}

	ret = snprintf(mnf->manufacture_line, MAX_CALIBRATION_STRING, "%u",
		eeprom->manufacture_line[0]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_ERROR("snprintf of mnf->manufacture_line failed");
		mnf->manufacture_line[0] = 0;
	}

	ret = snprintf(mnf->manufacture_date, MAX_CALIBRATION_STRING, "20%u/%u/%u",
		eeprom->manufacture_date[0], eeprom->manufacture_date[1], eeprom->manufacture_date[2]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_ERROR("snprintf of mnf->manufacture_date failed");
		mnf->manufacture_date[0] = 0;
	}

	ret = snprintf(mnf->serial_number, MAX_CALIBRATION_STRING, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		eeprom->serial_number[0], eeprom->serial_number[1],
		eeprom->serial_number[2], eeprom->serial_number[3],
		eeprom->serial_number[4], eeprom->serial_number[5],
		eeprom->serial_number[6], eeprom->serial_number[7],
		eeprom->serial_number[8], eeprom->serial_number[9],
		eeprom->serial_number[10], eeprom->serial_number[11],
		eeprom->serial_number[12], eeprom->serial_number[13],
		eeprom->serial_number[14], eeprom->serial_number[15]);
	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_ERROR("snprintf of mnf->serial_number failed");
		mnf->serial_number[0] = 0;
	}
}

void BOGOTA_GC32E1_eeprom_format_calibration_data(struct imgsensor_struct *pImgsensor)
{
	imgsensor = pImgsensor;
	BOGOTA_GC32E1_read_data_from_eeprom(BOGOTA_GC32E1_EEPROM_SLAVE_ADDR, 0x00, BOGOTA_GC32E1_EEPROM_SIZE);
	calibration_status.mnf = BOGOTA_GC32E1_check_manufacturing_data(BOGOTA_GC32E1_eeprom);
	BOGOTA_GC32E1_eeprom_get_mnf_data((void *)BOGOTA_GC32E1_eeprom, &mnf_info);
}

mot_calibration_status_t *BOGOTA_GC32E1_eeprom_get_calibration_status(void)
{
	return &calibration_status;
}

mot_calibration_mnf_t *BOGOTA_GC32E1_eeprom_get_mnf_info(void)
{
	return &mnf_info;
}
