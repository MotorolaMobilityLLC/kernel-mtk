/*****************************************************************************
 *
 * Filename:
 * ---------
 *   imx219otp.h
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
 *   Ronnie Lai (MTK01420)
 *
 *============================================================================*/
#ifndef __IMX219_OTP_H
#define __IMX219_OTP_H

#define CAM_CAL_DEV_MAJOR_NUMBER 226

/* CAM_CAL READ/WRITE ID */
#define IMX219OTP_DEVICE_ID	0x55
#define I2C_UNIT_SIZE                                  1 //in byte  
#define OTP_START_ADDR                            0x3B04
#define OTP_SIZE                                      24


#endif /* __IMX219_OTP_H */
