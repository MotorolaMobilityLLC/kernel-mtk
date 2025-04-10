#ifndef _EEPROM_I2C_OV08F_DRIVER_H_
#define _EEPROM_I2C_OV08F_DRIVER_H_

#define GROUP1_BLOCK_START_NUM 4
#define GROUP1_BLOCK_END_NUM   33

#define GROUP2_BLOCK_START_NUM 34
#define GROUP2_BLOCK_END_NUM   63

#define BLOCK_DATA_SIZE 128
#define LAST_BLOCK_DATA_SIZE 107

#define BASIC_INFO_SIZE 38
#define AWB_SIZE 44
#define OC_SIZE 17
#define SFR1_SIZE 42
#define SFR2_SIZE 5
#define QCOM_LSC_SIZE 1769
#define MTK_LSC_SIZE 1869
#define MTK_NECESSARY_INFO_SIZE 19


struct NEVADA_OV08F_otp_t {
	uint8_t flag_of_basic_infomation[1];
	uint8_t eeprom_table_revision[1];
	uint8_t cal_hw_version[1];
	uint8_t cal_sw_version[1];
	uint8_t moto_part_num[8];
	uint8_t actuator_id[1];
	uint8_t lens_id[1];
	uint8_t manufacturer_id[2];
	uint8_t factory_id[2];
	uint8_t manufacture_line[1];
	uint8_t manufacture_date[3];
	uint8_t serial_number[16];
	uint8_t manufacturing_data_chksum[2];

	uint8_t flag_of_awb[1];
	uint8_t awb_param[43];
	uint8_t awb_chksum[2];

	uint8_t flag_of_oc[1];
	uint8_t oc_param[16];
	uint8_t oc_chksum[2];

	uint8_t flag_of_sfr1[1];
	uint8_t sfr1_param[41];
	uint8_t sfr1_chksum[2];

	uint8_t flag_of_sfr2[1];
	uint8_t sfr2_param[4];
	uint8_t sfr2_chksum[2];

	uint8_t flag_of_qcom_lsc[1];
	uint8_t qcom_lsc_param[1768];
	uint8_t qcom_lsc_chksum[2];

	uint8_t flag_of_mtk_lsc[1];
	uint8_t mtk_lsc_param[1868];
	uint8_t mtk_lsc_chksum[2];

	uint8_t flag_of_mtk_necessary_info[1];
	uint8_t module_check_flag[1];
	uint8_t otp_cali_type[4];
	uint8_t sensor_type[1];
	uint8_t awb_af_cali_info[1];
	uint8_t one_table_size[2];
	uint8_t af_inf_cali[2];
	uint8_t af_macro_cali[2];
	uint8_t new_mtk_format_version[1];
	uint8_t new_mtk_format_flag[3];
	uint8_t af_inf_cali_temperature[1];
	uint8_t mtk_necessary_info_chksum[2];
};

#endif // _EEPROM_I2C_OV08F_DRIVER_H_
