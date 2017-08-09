/*****************************************************************************
 *
 * Filename:
 * ---------
 *   S-24CS64A.h
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 *   Header file of EEPROM driver
 *
 *
 * Author:
 * -------
 *   Ronnie Lai (MTK01420)
 *
 *============================================================================*/
#ifndef __EEPROM_H
#define __EEPROM_H

#define EEPROM_DEV_MAJOR_NUMBER 226

/* EEPROM READ/WRITE ID */
#define S24CS64A_DEVICE_ID							0xAA

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iBurstReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern int iReadReg(u16 a_u2Addr , u8 *a_puBuff , u16 i2cId);

#endif /* __EEPROM_H */

