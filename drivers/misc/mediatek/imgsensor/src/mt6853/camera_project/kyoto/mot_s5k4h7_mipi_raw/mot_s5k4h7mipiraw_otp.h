
/*
 * Copyright (C) 2021 lucas (guoqiang8@lenovo.com).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef _S5K4H7MIPI_OTP_H
#define _S5K4H7MIPI_OTP_H

#define DATA_PAGE_NUM 3
#define OTP_PAGE_START_ADDR 0x0A04
#define OTP_PAGE_SIZE 64

typedef enum {
	PAGE_INVAIL,
	PAGE_VAIL,
        PAGE_VAIL_BIG
} CHECK_PAGE;

typedef struct {
	unsigned short page_num;
	unsigned short page_sart_size;
	unsigned short page_end_size;
} PAGE_OFFSETINFO;

typedef struct {
	unsigned short page[DATA_PAGE_NUM];
	unsigned short page_vail;
        PAGE_OFFSETINFO page_offset[DATA_PAGE_NUM];;
        unsigned int   page_addr[DATA_PAGE_NUM];
	CHECK_PAGE     flag;

} PAGE_INFO;

typedef struct {
	PAGE_INFO page_info;
	unsigned int size;
	unsigned char *data;
} MNF_INFO;

typedef struct {
	PAGE_INFO page_info;
	unsigned int size;
	unsigned char *data;
} AWB_INFO;

typedef struct {
	PAGE_INFO page_info;
	unsigned int size;
	unsigned char *data;
} AF_INFO;

typedef struct {
	PAGE_INFO page_info;
	unsigned int size;
	unsigned char *data;
} AF_SYNC_INFO;

typedef struct {
	PAGE_INFO page_info;
	unsigned int size;
	unsigned char *data;
} LSC_INFO;

typedef struct {
	MNF_INFO  mnf_info;
	AWB_INFO  awb_info;
	AF_INFO   af_info;
	LSC_INFO  lsc_info;
	unsigned char data[2048];
} OTP_INFO;

extern int iReadRegI2C(u8 *a_pSendData, u16 a_sizeSendData,
	u8 *a_pRecvData, u16 a_sizeRecvData,u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u16 i2cId);
extern int iReadRegI2CTiming(u8 *a_pSendData, u16 a_sizeSendData,
	u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId, u16 timing);
extern int iWriteRegI2CTiming(u8 *a_pSendData, u16 a_sizeSendData,
	u16 i2cId, u16 timing);

extern void kdSetI2CSpeed(u16 i2cSpeed);
extern int iReadReg(u16 a_u2Addr, u8 *a_puBuff, u16 i2cId);
extern int iWriteReg(u16 a_u2Addr, u32 a_u4Data, u32 a_u4Bytes, u16 i2cId);
extern int iBurstWriteReg_multi(u8 *pData, u32 bytes, u16 i2cId,
				u16 transfer_length, u16 timing);
int s5k4h7_otp_data(void);
#endif
