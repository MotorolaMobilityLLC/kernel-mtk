/*****************************************************************************
 *
 * Filename:
 * ---------
 *   catc24c16.h
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 *   Header file of CAM_CAL driver
 *
 *
 * Author:
 * -------
 *   LukeHu
 *
 *============================================================================*/
#ifndef __IMX258_EEPROM_H
#define __IMX258_EEPROM_H

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 *a_pRecvData,
u16 a_sizeRecvData, u16 i2cId);



#endif /* __CAM_CAL_H */

