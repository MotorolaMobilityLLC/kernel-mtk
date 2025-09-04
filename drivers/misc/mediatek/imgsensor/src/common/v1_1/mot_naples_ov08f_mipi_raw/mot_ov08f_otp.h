#ifndef _EEPROM_I2C_OV08F_DRIVER_H_
#define _EEPROM_I2C_OV08F_DRIVER_H_

#define GROUP1_BLOCK_START_NUM 4
#define GROUP1_BLOCK_END_NUM   19

#define GROUP2_BLOCK_START_NUM 19
#define GROUP2_BLOCK_END_NUM   34

#define GROUP3_BLOCK_START_NUM 35
#define GROUP3_BLOCK_END_NUM   50

#define BLOCK_DATA_SIZE 128
#define GROUP1_LAST_BLOCK_DATA_SIZE 9
#define GROUP2_LAST_BLOCK_DATA_SIZE 89
#define GROUP3_LAST_BLOCK_DATA_SIZE 41

#define BASIC_INFO_SIZE 37
#define AWB_SIZE 17
#define MTK_LSC_SIZE 1869
#define MTK_NECESSARY_INFO_SIZE 0
#define ALL_DATA_SIZE 1928

struct NAPLES_OV08F_otp_t {
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
	uint8_t awb_data[17];
	uint8_t flag_of_mtk_lsc[1];
	uint8_t mtk_lsc_info[1869];
	uint8_t all_data_chksum[1];
};

#endif // _EEPROM_I2C_OV08F_DRIVER_H_
