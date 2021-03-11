////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Motorola Mobility, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Motorola Mobility, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __OV02B1BMIPI_OTP_H__
#define __OV02B1BMIPI_OTP_H__

#define DUMP_DEPTH_SERIAL_NUMBER_SIZE 16

#define EEPROM_DATA_PATH "/data/vendor/camera_dump/ov02b1b_eeprom_data.bin"
#define SERIAL_DATA_PATH "/data/vendor/camera_dump/serial_number_depth.bin"

enum ov02b1b_page_number {
    PAGE_0,
    PAGE_1,
    PAGE_MAX
};

enum ov02b1b_program_flag {
    PGM_FLAG_EMPTY = 0,
    PGM_FLAG_VALID1 = 1,
    PGM_FLAG_VALID2 = 1 << 6,
    PGM_FLAG_INVALID = 3 << 6
};

typedef struct mnf_info_group_t {
    UINT8 valid_flag[1];
    UINT8 table_revision[1];
    UINT8 manufacturer_id[2];
    UINT8 manufacture_line[1];
    UINT8 manufacture_date[3];
} mnf_info_group;

typedef struct mot_ov02b1b_otp_map_t {
    UINT8 serial_number[16];
    mnf_info_group mnf_info[2];
} mot_ov02b1b_otp;

extern int iReadRegI2C(u8 *a_pSendData, u16 a_sizeSendData,
	u8 *a_pRecvData, u16 a_sizeRecvData,u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u16 i2cId);

extern void kdSetI2CSpeed(u16 i2cSpeed);
extern int iReadReg(u16 a_u2Addr, u8 *a_puBuff, u16 i2cId);
extern int iWriteReg(u16 a_u2Addr, u32 a_u4Data, u32 a_u4Bytes, u16 i2cId);
extern int iBurstWriteReg_multi(u8 *pData, u32 bytes, u16 i2cId,
				u16 transfer_length, u16 timing);
#endif
