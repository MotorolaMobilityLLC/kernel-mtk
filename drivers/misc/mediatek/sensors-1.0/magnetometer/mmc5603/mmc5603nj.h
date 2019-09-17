/*****************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software is protected by Copyright and the information and source code
 *  contained herein is confidential. The software including the source code
 *  may not be copied and the information contained herein may not be used or
 *  disclosed except with the written permission of MEMSIC Inc. (C) 2017
 *****************************************************************************/

/*
 * mmc5603nj.h - Definitions for mmc5603nj magnetic sensor chip.
 */

#ifndef __MMC5603NJ_H__
#define __MMC5603NJ_H__

#include <linux/ioctl.h>

#define CALIBRATION_DATA_SIZE 12


#define MMC5603NJ_I2C_ADDR 0x30 // 7-bit

/*address bit*/
#define MMC5603NJ_REG_DATA 0x00
#define MMC5603NJ_REG_TEMP 0x09
#define MMC5603NJ_REG_STATUS1 0x18
#define MMC5603NJ_REG_STATUS0 0x19
#define MMC5603NJ_REG_ODR 0x1A
#define MMC5603NJ_REG_CTRL0 0x1B
#define MMC5603NJ_REG_CTRL1 0x1C
#define MMC5603NJ_REG_CTRL2 0x1D
#define MMC5603NJ_REG_PRODUCTID 0x39

/*bit value*/
#define MMC5603NJ_CMD_AUTO_SR 0xa0
#define MMC5603NJ_CMD_RESET 0x10
#define MMC5603NJ_CMD_SET 0x08
#define MMC5603NJ_CMD_TM_M 0x01
#define MMC5603NJ_CMD_TM_T 0x02
#define MMC5603NJ_CMD_BW_01 0x01
#define MMC5603NJ_CMD_CM 0x10
#define MMC5603NJ_PRODUCT_ID 0x10

#define MMC5603NJ_NUM_AXES           3
#define MMC5603NJ_CMD_TMM                 0x01
#define MMC5603NJ_CMD_TMT                 0x02
#define MMC5603NJ_CMD_START_MDT           0x04
#define MMC5603NJ_CMD_SET                 0x08
#define MMC5603NJ_CMD_RESET               0x10
#define MMC5603NJ_CMD_AUTO_SR_EN          0x20
#define MMC5603NJ_CMD_AUTO_ST_EN          0x40
#define MMC5603NJ_CMD_CMM_FREQ_EN         0x80
#define MMC5603NJ_CMD_CMM_EN              0x10

#define MMC5603NJ_REG_X_THD               0x1E
#define MMC5603NJ_REG_Y_THD               0x1F
#define MMC5603NJ_REG_Z_THD               0x20

#define MMC5603NJ_REG_ST_X_VAL            0x27
#define MMC5603NJ_REG_ST_Y_VAL            0x28
#define MMC5603NJ_REG_ST_Z_VAL            0x29
#define MMC5603NJ_SAT_SENSOR              0x20


// 16-bit mode, null field output (32768)
#define MMC5603NJ_OFFSET 32768
#define MMC5603NJ_SENSITIVITY 1024

#define MMC5603NJ_SAMPLE_RATE 100

#define CONVERT_M_DIV 1


#define COMPAT_MMC5603NJ_IOC_READ_REG _IOWR(MSENSOR, 0x32, unsigned char)
#define COMPAT_MMC5603NJ_IOC_WRITE_REG _IOW(MSENSOR, 0x33, unsigned char[2])
#define COMPAT_MMC5603NJ_IOC_READ_REGS _IOWR(MSENSOR, 0x34, unsigned char[10])
#define MMC5603NJ_IOC_READ_REG _IOWR(MSENSOR, 0x23, unsigned char)
#define MMC5603NJ_IOC_WRITE_REG _IOW(MSENSOR, 0x24, unsigned char[2])
#define MMC5603NJ_IOC_READ_REGS _IOWR(MSENSOR, 0x25, unsigned char[10])

#endif /* __MMC5603NJ_H__ */
