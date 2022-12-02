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

#include "mot_lyriq_ov50amipiraw_Sensor.h"

static int mot_sensor_debug = 1;

typedef struct {
	MUINT16 addr;
	MUINT16 data;
} lyriq_ois_cal_addr_data_t;

#define PFX "MOT_LYRIQ_OV50A"
#define LOG_INF(format, args...)        do { if (mot_sensor_debug   ) { pr_err(PFX "[%s] " format, __func__,##args); } } while(0)
#define LOG_ERR(format, args...)        do { if (mot_sensor_debug   ) { pr_err(PFX "[%s] " format, __func__,##args); } } while(0)
#define LOG_INF_N(format, args...)   pr_warn(PFX "[%s] " format, __func__, ##args)
#define LOG_ERROR(format, args...)   pr_err(PFX "[%s] " format, __func__, ##args)

static DEFINE_SPINLOCK(imgsensor_lock);
static  struct imgsensor_struct *imgsensor;

#define LYRIQ_OV50A_EEPROM_SLAVE_ADDR 0xA0
#define LYRIQ_OV50A_SENSOR_IIC_SLAVE_ADDR 0x20
#define LYRIQ_OV50A_EEPROM_SIZE  0x2898
#define LYRIQ_OV50A_EEPROM_CRC_MANUFACTURING_SIZE 37
#define LYRIQ_OV50A_EEPROM_CRC_AF_CAL_SIZE 24
#define LYRIQ_OV50A_EEPROM_CRC_AWB_CAL_SIZE 43
#define LYRIQ_OV50A_EEPROM_CRC_LSC_SIZE 1868
#define LYRIQ_OV50A_EEPROM_CRC_PDAF_OUTPUT1_SIZE 496
#define LYRIQ_OV50A_EEPROM_CRC_PDAF_OUTPUT2_SIZE 1004
#define LYRIQ_OV50A_EEPROM_CRC_QPD_SIZE 3568
#define LYRIQ_OV50A_EEPROM_CRC_OIS_SR_SIZE 22
#define LYRIQ_OV50A_EEPROM_CRC_AK7323_OIS_SIZE 168
#define LYRIQ_OV50A_EEPROM_CRC_AF_SYNC_SIZE 16

static lyriq_ois_cal_addr_data_t ois_sr_data[LYRIQ_OV50A_EEPROM_CRC_OIS_SR_SIZE] = {{0}};
static lyriq_ois_cal_addr_data_t ak7323_ois_data[LYRIQ_OV50A_EEPROM_CRC_AK7323_OIS_SIZE] = {{0}};

static uint8_t LYRIQ_OV50A_eeprom[LYRIQ_OV50A_EEPROM_SIZE] = {0};
static mot_calibration_status_t calibration_status = {CRC_FAILURE};
static mot_calibration_mnf_t mnf_info = {0};

extern kal_uint16 mot_lyriq_ov50a_table_write_cmos_sensor(kal_uint16 *para, kal_uint32 len);

static uint8_t crc_reverse_byte(uint32_t data)
{
	return ((data * 0x0802LU & 0x22110LU) |
		(data * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}

static uint32_t convert_crc(uint8_t *crc_ptr)
{
	return (crc_ptr[0] << 8) | (crc_ptr[1]);
}

static uint16_t to_uint16_swap(uint8_t *data)
{
	uint16_t converted;
	memcpy(&converted, data, sizeof(uint16_t));
	return ntohs(converted);
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

static kal_uint16 LYRIQ_OV50A_read_cmos_sensor_8(kal_uint16 addr)
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

static void LYRIQ_OV50A_read_data_from_eeprom(kal_uint8 slave, kal_uint32 start_add, uint32_t size)
{
	int i = 0;
	spin_lock(&imgsensor_lock);
	imgsensor->i2c_write_id = slave;
	spin_unlock(&imgsensor_lock);

	for (i = 0; i < size; i ++) {
		LYRIQ_OV50A_eeprom[i] = LYRIQ_OV50A_read_cmos_sensor_8(start_add);
		start_add ++;
	}

	spin_lock(&imgsensor_lock);
	imgsensor->i2c_write_id = LYRIQ_OV50A_SENSOR_IIC_SLAVE_ADDR;
	spin_unlock(&imgsensor_lock);
}

static calibration_status_t LYRIQ_OV50A_check_manufacturing_data(void *data)
{
	struct LYRIQ_OV50A_eeprom_t *eeprom = (struct LYRIQ_OV50A_eeprom_t*)data;
	LOG_INF("Manufacturing eeprom->mpn = %s !",eeprom->mpn);
	if (!eeprom_util_check_crc16(eeprom->eeprom_table_version, LYRIQ_OV50A_EEPROM_CRC_MANUFACTURING_SIZE,
		convert_crc(eeprom->manufacture_crc16))) {
		LOG_ERROR("Manufacturing CRC Fails!");
		return CRC_FAILURE;
	}
	LOG_INF("Manufacturing CRC Pass");
	return NO_ERRORS;
}

static calibration_status_t LYRIQ_OV50A_check_af_data(void *data)
{
	struct LYRIQ_OV50A_eeprom_t *eeprom = (struct LYRIQ_OV50A_eeprom_t*)data;

	if (!eeprom_util_check_crc16(eeprom->af_data, LYRIQ_OV50A_EEPROM_CRC_AF_CAL_SIZE,
		convert_crc(eeprom->af_crc16))) {
		LOG_ERROR("Autofocus CRC Fails!");
		return CRC_FAILURE;
	}
	LOG_INF("Autofocus CRC Pass");
	return NO_ERRORS;
}

static uint8_t mot_eeprom_util_calculate_awb_factors_limit(awb_t unit, awb_t golden,
		awb_limit_t limit)
{
	uint32_t r_g;
	uint32_t b_g;
	uint32_t golden_rg, golden_bg;
	uint32_t r_g_golden_min;
	uint32_t r_g_golden_max;
	uint32_t b_g_golden_min;
	uint32_t b_g_golden_max;

	r_g = unit.r_g * 1000;
	b_g = unit.b_g*1000;

	golden_rg = golden.r_g* 1000;
	golden_bg = golden.b_g* 1000;

	r_g_golden_min = limit.r_g_golden_min*16384;
	r_g_golden_max = limit.r_g_golden_max*16384;
	b_g_golden_min = limit.b_g_golden_min*16384;
	b_g_golden_max = limit.b_g_golden_max*16384;
	LOG_INF("rg = %d, bg=%d,rgmin=%d,bgmax =%d\n",r_g,b_g,r_g_golden_min,r_g_golden_max);
	LOG_INF("grg = %d, gbg=%d,bgmin=%d,bgmax =%d\n",golden_rg,golden_bg,b_g_golden_min,b_g_golden_max);
	if (r_g < (golden_rg - r_g_golden_min) || r_g > (golden_rg + r_g_golden_max)) {
		LOG_INF("Final RG calibration factors out of range!");
		return 1;
	}

	if (b_g < (golden_bg - b_g_golden_min) || b_g > (golden_bg + b_g_golden_max)) {
		LOG_INF("Final BG calibration factors out of range!");
		return 1;
	}
	return 0;
}

static uint8_t mot_eeprom_util_check_awb_limits(awb_t unit, awb_t golden)
{
	uint8_t result = 0;

	if (unit.r < AWB_R_MIN || unit.r > AWB_R_MAX) {
		LOG_INF("unit r out of range! MIN: %d, r: %d, MAX: %d",
			AWB_R_MIN, unit.r, AWB_R_MAX);
		result = 1;
	}
	if (unit.gr < AWB_GR_MIN || unit.gr > AWB_GR_MAX) {
		LOG_INF("unit gr out of range! MIN: %d, gr: %d, MAX: %d",
			AWB_GR_MIN, unit.gr, AWB_GR_MAX);
		result = 1;
	}
	if (unit.gb < AWB_GB_MIN || unit.gb > AWB_GB_MAX) {
		LOG_INF("unit gb out of range! MIN: %d, gb: %d, MAX: %d",
			AWB_GB_MIN, unit.gb, AWB_GB_MAX);
		result = 1;
	}
	if (unit.b < AWB_B_MIN || unit.b > AWB_B_MAX) {
		LOG_INF("unit b out of range! MIN: %d, b: %d, MAX: %d",
			AWB_B_MIN, unit.b, AWB_B_MAX);
		result = 1;
	}

	if (golden.r < AWB_R_MIN || golden.r > AWB_R_MAX) {
		LOG_INF("golden r out of range! MIN: %d, r: %d, MAX: %d",
			AWB_R_MIN, golden.r, AWB_R_MAX);
		result = 1;
	}
	if (golden.gr < AWB_GR_MIN || golden.gr > AWB_GR_MAX) {
		LOG_INF("golden gr out of range! MIN: %d, gr: %d, MAX: %d",
			AWB_GR_MIN, golden.gr, AWB_GR_MAX);
		result = 1;
	}
	if (golden.gb < AWB_GB_MIN || golden.gb > AWB_GB_MAX) {
		LOG_INF("golden gb out of range! MIN: %d, gb: %d, MAX: %d",
			AWB_GB_MIN, golden.gb, AWB_GB_MAX);
		result = 1;
	}
	if (golden.b < AWB_B_MIN || golden.b > AWB_B_MAX) {
		LOG_INF("golden b out of range! MIN: %d, b: %d, MAX: %d",
			AWB_B_MIN, golden.b, AWB_B_MAX);
		result = 1;
	}

	return result;
}

static calibration_status_t LYRIQ_OV50A_check_awb_data(void *data)
{
	struct LYRIQ_OV50A_eeprom_t *eeprom = (struct LYRIQ_OV50A_eeprom_t*)data;
	awb_t unit;
	awb_t golden;
	awb_limit_t golden_limit;

	if(!eeprom_util_check_crc16(eeprom->cie_src_1_ev,
		LYRIQ_OV50A_EEPROM_CRC_AWB_CAL_SIZE,
		convert_crc(eeprom->awb_crc16))) {
		LOG_ERROR("AWB CRC Fails!");
		return CRC_FAILURE;
	}

	unit.r = to_uint16_swap(eeprom->awb_src_1_r)/64;
	unit.gr = to_uint16_swap(eeprom->awb_src_1_gr)/64;
	unit.gb = to_uint16_swap(eeprom->awb_src_1_gb)/64;
	unit.b = to_uint16_swap(eeprom->awb_src_1_b)/64;
	unit.r_g = to_uint16_swap(eeprom->awb_src_1_rg_ratio);
	unit.b_g = to_uint16_swap(eeprom->awb_src_1_bg_ratio);
	unit.gr_gb = to_uint16_swap(eeprom->awb_src_1_gr_gb_ratio);

	golden.r = to_uint16_swap(eeprom->awb_src_1_golden_r)/64;
	golden.gr = to_uint16_swap(eeprom->awb_src_1_golden_gr)/64;
	golden.gb = to_uint16_swap(eeprom->awb_src_1_golden_gb)/64;
	golden.b = to_uint16_swap(eeprom->awb_src_1_golden_b)/64;
	golden.r_g = to_uint16_swap(eeprom->awb_src_1_golden_rg_ratio);
	golden.b_g = to_uint16_swap(eeprom->awb_src_1_golden_bg_ratio);
	golden.gr_gb = to_uint16_swap(eeprom->awb_src_1_golden_gr_gb_ratio);
	if (mot_eeprom_util_check_awb_limits(unit, golden)) {
		LOG_ERROR("AWB CRC limit Fails!");
		return LIMIT_FAILURE;
	}

	golden_limit.r_g_golden_min = eeprom->awb_r_g_golden_min_limit[0];
	golden_limit.r_g_golden_max = eeprom->awb_r_g_golden_max_limit[0];
	golden_limit.b_g_golden_min = eeprom->awb_b_g_golden_min_limit[0];
	golden_limit.b_g_golden_max = eeprom->awb_b_g_golden_max_limit[0];

	if (mot_eeprom_util_calculate_awb_factors_limit(unit, golden,golden_limit)) {
		LOG_ERROR("AWB CRC factor limit Fails!");
		return LIMIT_FAILURE;
	}
	LOG_INF("AWB CRC Pass");
	return NO_ERRORS;
}

static calibration_status_t LYRIQ_OV50A_check_lsc_data_mtk(void *data)
{
	struct LYRIQ_OV50A_eeprom_t *eeprom = (struct LYRIQ_OV50A_eeprom_t*)data;

	if (!eeprom_util_check_crc16(eeprom->lsc_data_mtk, LYRIQ_OV50A_EEPROM_CRC_LSC_SIZE,
		convert_crc(eeprom->lsc_crc16_mtk))) {
		LOG_INF("LSC CRC Fails!");
		return CRC_FAILURE;
	}
	LOG_INF("LSC CRC Pass");
	return NO_ERRORS;
}

static calibration_status_t LYRIQ_OV50A_check_pdaf_data(void *data)
{
	struct LYRIQ_OV50A_eeprom_t *eeprom = (struct LYRIQ_OV50A_eeprom_t*)data;

	if (!eeprom_util_check_crc16(eeprom->pdaf_out1_data_mtk, LYRIQ_OV50A_EEPROM_CRC_PDAF_OUTPUT1_SIZE,
		convert_crc(eeprom->pdaf_out1_crc16_mtk))) {
		LOG_ERROR("PDAF OUTPUT1 CRC Fails!");
		return CRC_FAILURE;
	}

	if (!eeprom_util_check_crc16(eeprom->pdaf_out2_data_mtk, LYRIQ_OV50A_EEPROM_CRC_PDAF_OUTPUT2_SIZE,
		convert_crc(eeprom->pdaf_out2_crc16_mtk))) {
		LOG_ERROR("PDAF OUTPUT2 CRC Fails!");
		return CRC_FAILURE;
	}

	LOG_INF("PDAF CRC Pass");
	return NO_ERRORS;
}

static calibration_status_t LYRIQ_OV50A_check_af_sync_data(void *data)
{
        struct LYRIQ_OV50A_eeprom_t *eeprom = (struct LYRIQ_OV50A_eeprom_t*)data;

        if (!eeprom_util_check_crc16(eeprom->af_sync_data, LYRIQ_OV50A_EEPROM_CRC_AF_SYNC_SIZE,
                convert_crc(eeprom->af_sync_crc))) {
                LOG_INF("AF SYNC CRC Fails!");
                return CRC_FAILURE;
        }
        LOG_INF("AF SYNC CRC Pass");
        return NO_ERRORS;
}

static void LYRIQ_OV50A_eeprom_get_mnf_data(void *data,
		mot_calibration_mnf_t *mnf)
{
	int ret;
	struct LYRIQ_OV50A_eeprom_t *eeprom = (struct LYRIQ_OV50A_eeprom_t*)data;

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

	ret = snprintf(mnf->lens_id, MAX_CALIBRATION_STRING, "0x%x", eeprom->lens_id[0]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_ERROR("snprintf of mnf->lens_id failed");
		mnf->lens_id[0] = 0;
	}

	if (eeprom->manufacturer_id[0] == 'S' && eeprom->manufacturer_id[1] == 'U') {
		ret = snprintf(mnf->integrator, MAX_CALIBRATION_STRING, "Sunny");
	} else if (eeprom->manufacturer_id[0] == 'O' && eeprom->manufacturer_id[1] == 'F') {
		ret = snprintf(mnf->integrator, MAX_CALIBRATION_STRING, "OFilm");
	} else if (eeprom->manufacturer_id[0] == 'Q' && eeprom->manufacturer_id[1] == 'T') {
		ret = snprintf(mnf->integrator, MAX_CALIBRATION_STRING, "Qtech");
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

int get_ois_sr_data(void *data)
{
	int i;
	struct LYRIQ_OV50A_eeprom_t *eeprom = (struct LYRIQ_OV50A_eeprom_t*)data;
	if (!eeprom_util_check_crc16(eeprom->ois_sr_data, LYRIQ_OV50A_EEPROM_CRC_OIS_SR_SIZE,
		convert_crc(eeprom->ois_sr_crc)))
	{
		pr_debug("Ois Sr Data CRC Fail!");
	}
	else
	{
		pr_debug("Ois Sr Data CRC Pass");
		for (i = 0; i < LYRIQ_OV50A_EEPROM_CRC_OIS_SR_SIZE; i++)
		{
			ois_sr_data[i].addr = 0x2337 + i;
			ois_sr_data[i].data = eeprom->ois_sr_data[i];
		}
	}

	return 1;
}

int get_ak7323_ois_data(void *data)
{
	int i;
	struct LYRIQ_OV50A_eeprom_t *eeprom = (struct LYRIQ_OV50A_eeprom_t*)data;
	if (!eeprom_util_check_crc16(eeprom->ak7323_ois_data, LYRIQ_OV50A_EEPROM_CRC_AK7323_OIS_SIZE,
		convert_crc(eeprom->ak7323_ois_crc)))
	{
		pr_debug("AK7323 OIS Data CRC Fail!");
	}
	else
	{
		pr_debug("AK7323 OIS Data CRC Pass");
		for (i = 0; i < LYRIQ_OV50A_EEPROM_CRC_AK7323_OIS_SIZE; i++)
		{
			ak7323_ois_data[i].addr = 0x234F + i;
			ak7323_ois_data[i].data = eeprom->ak7323_ois_data[i];
		}
	}
	return 1;
}

void LYRIQ_OV50A_eeprom_format_calibration_data(struct imgsensor_struct *pImgsensor)
{
	imgsensor = pImgsensor;
	LYRIQ_OV50A_read_data_from_eeprom(LYRIQ_OV50A_EEPROM_SLAVE_ADDR, 0x00, LYRIQ_OV50A_EEPROM_SIZE);
	calibration_status.mnf = LYRIQ_OV50A_check_manufacturing_data(LYRIQ_OV50A_eeprom);
	calibration_status.af = LYRIQ_OV50A_check_af_data(LYRIQ_OV50A_eeprom);
	calibration_status.awb = LYRIQ_OV50A_check_awb_data(LYRIQ_OV50A_eeprom);
	calibration_status.lsc = LYRIQ_OV50A_check_lsc_data_mtk(LYRIQ_OV50A_eeprom);
	calibration_status.pdaf = LYRIQ_OV50A_check_pdaf_data(LYRIQ_OV50A_eeprom);
	calibration_status.af_sync = LYRIQ_OV50A_check_af_sync_data(LYRIQ_OV50A_eeprom);
	calibration_status.dual = NO_ERRORS;

	LYRIQ_OV50A_eeprom_get_mnf_data((void *)LYRIQ_OV50A_eeprom, &mnf_info);

	get_ak7323_ois_data(LYRIQ_OV50A_eeprom);
	get_ak7323_ois_data(LYRIQ_OV50A_eeprom);

	LOG_INF("status mnf:%d, af:%d, awb:%d, lsc:%d, pdaf:%d, af_sync:%d, dual:%d",
	        calibration_status.mnf, calibration_status.af, calibration_status.awb,
	        calibration_status.lsc, calibration_status.pdaf, calibration_status.af_sync, calibration_status.dual);
}

mot_calibration_status_t *LYRIQ_OV50A_eeprom_get_calibration_status(void)
{
	return &calibration_status;
}

mot_calibration_mnf_t *LYRIQ_OV50A_eeprom_get_mnf_info(void)
{
	return &mnf_info;
}
/*
void write_cross_talk_data(void)
{
	uint16_t write_table[LYRIQ_OV50A_EEPROM_CRC_XTALK_SIZE * 2] = {0};

	pr_debug("E\n");

	memcpy(write_table, &ov_cross_talk_data[0].addr, sizeof(write_table));

	mot_lyriq_ov50a_table_write_cmos_sensor(write_table,
		sizeof(write_table)/sizeof(uint16_t));

	pr_debug("apply cross talk calibration data success.");
	pr_debug("X");
}

void write_pdc_data(void)
{
	uint16_t write_table[LYRIQ_OV50A_EEPROM_CRC_PDC_WRITE_SIZE * 2] = {0};

	pr_debug("E\n");

	memcpy(write_table, &ov_pdc_data[0].addr, sizeof(write_table));

	mot_lyriq_ov50a_table_write_cmos_sensor(write_table,
		sizeof(write_table)/sizeof(uint16_t));

	pr_debug("apply pdc calibration data success.");
	pr_debug("X");
}
*/
