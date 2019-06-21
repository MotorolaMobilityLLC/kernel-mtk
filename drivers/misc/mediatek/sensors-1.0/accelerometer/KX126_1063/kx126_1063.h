 
  /* (C) Copyright 2008
 * MediaTek <www.mediatek.com>
 *
 * KX126_1063 driver for MT6737
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA  KX126
 */
#ifndef KX126_1063_H
#define KX126_1063_H

#include <linux/ioctl.h>

/*---------settings for I2C access interface----------*/
#define KXTJ_REG_ADDR_LEN	1
#define KXTJ_DMA_MAX_TRANSACTION_LEN  255	/* for DMA mode */

#ifdef CONFIG_MTK_I2C_EXTENSION	/*for ARCH_MT6735,MT6735M, MT6753,MT6580,MT6755 */
#define KXTJ_ENABLE_I2C_DMA	/*DMA mode */
#define KXTJ_I2C_MASTER_CLOCK        100	/*I2C clock, default 100kHz */
#ifdef KXTJ_ENABLE_I2C_DMA
#define KXTJ_ENABLE_WRRD_MODE	/*WRRD(write and read) mode */
#define KXTJ_DMA_MAX_WR_SIZE	(KXTJ_DMA_MAX_TRANSACTION_LEN - KXTJ_REG_ADDR_LEN)
#ifdef KXTJ_ENABLE_WRRD_MODE	/*for WRRD(write and read) mode */
#define KXTJ_DMA_MAX_RD_SIZE	31
#else
#define KXTJ_DMA_MAX_RD_SIZE	KXTJ_DMA_MAX_TRANSACTION_LEN
#endif
#else	/*RPR_ENABLE_I2C_DMA*/			/*FIFO mode */
#define KXTJ_FIFO_MAX_RD_SIZE C_I2C_FIFO_SIZE
#define KXTJ_FIFO_MAX_WR_SIZE C_I2C_FIFO_SIZE - KXTJ_REG_ADDR_LEN
#endif
#else				/*CONFIG_MTK_I2C_EXTENSION */
#define KXTJ_DMA_MAX_RD_SIZE	31
#define KXTJ_DMA_MAX_WR_SIZE	(KXTJ_DMA_MAX_TRANSACTION_LEN - KXTJ_REG_ADDR_LEN)
#endif
/*-----------------end of I2C interface------------------*/


#define KX126_1063_I2C_SLAVE_ADDR		0x3C

 /* KX126_1063 Register Map  (Please refer to KX126_1063 Specifications) */
#define KX126_1063_REG_DEVID			0x011
#define	KX126_1063_REG_BW_RATE			0x1F
#define KX126_1063_REG_INT_SOURCE1	0x15
#define KX126_1063_REG_INT_SOURCE2	0X16
#define KX126_1063_REG_INT_REL		0x19

#define KX126_1063_REG_CTL_REG1		0x1A
#define KX126_1063_REG_DATA_FORMAT		0x1A
#define KX126_1063_REG_DATA_RESOLUTION		0x1A

#define KX126_1063_RANGE_DATA_RESOLUTION_MASK	0x40
#define KX126_1063_RANGE_MASK		0x1C
#define KX126_1063_RANGE_2G			0x00
#define KX126_1063_RANGE_4G			0x08
#define KX126_1063_RANGE_8G			0x10
#define KX126_1063_RANGE_16G		0x1C
#define KX126_1063_WUFE_ENABLE	0x02
#define KX126_1063_DRDYE_ENABLE	0x20
#define KX126_1063_MEASURE_MODE	0x80

#define KX126_1063_REG_CTL_REG2		0x1B
#define KX126_1063_REG_INT_CTL_REG1	0x20
#define KX126_1063_REG_INT_CTL_REG2	0x21
#define KX126_1063_DCST_RESP			0x0C
#define KX126_1063_REG_DATAX0			0x08
#define KX126_1063_REG_WAKEUP_TIMER	0x40
#define KX126_1063_REG_NA_COUNTER	0x2A
/*0x6A:(bit11-bit4), 0x6B:(bit7:bit4)*/
#define KX126_1063_REG_WAKEUP_THRESHOLD_H	0x3C
#define KX126_1063_REG_WAKEUP_THRESHOLD_L	0x3D

#define KX126_1063_FIXED_DEVID			0x38
#define KX126_1063_BW_200HZ				0x04
#define KX126_1063_BW_100HZ				0x03
#define KX126_1063_BW_50HZ				0x02
#define KX126_1063_SELF_TEST           0x10



//for pedometer

#define PED_STP_L             0x0E
#define INC1           	  	  0x20
#define INC5           	  	  0x24
#define INC7           	  	  0x26
#define LP_CNTL           	  0x37
#define PED_STPWM_L           0x41
#define PED_STPWM_H           0x42 
#define PED_CNTL1	          0x43
#define PED_CNTL2	          0x44
#define PED_CNTL3             0x45
#define PED_CNTL4             0x46 
#define PED_CNTL5	          0x47
#define PED_CNTL6	          0x48
#define PED_CNTL7             0x49 
#define PED_CNTL8	          0x4A
#define PED_CNTL9	          0x4B
#define PED_CNTL10	          0x4C
//end pedometer

#define KX126_1063_SUCCESS						0
#define KX126_1063_ERR_I2C						-1
#define KX126_1063_ERR_STATUS					-3
#define KX126_1063_ERR_SETUP_FAILURE				-4
#define KX126_1063_ERR_GETGSENSORDATA			-5
#define KX126_1063_ERR_IDENTIFICATION			-6



#define KX126_1063_BUFSIZE				256
#define KX126_1063_AXES_NUM        3

/*----------------------------------------------------------------------------*/
typedef enum {
	KX126_1063_CUST_ACTION_SET_CUST = 1,
	KX126_1063_CUST_ACTION_SET_CALI,
	KX126_1063_CUST_ACTION_RESET_CALI
} CUST_ACTION;
/*----------------------------------------------------------------------------*/
typedef struct {
	uint16_t action;
} KX126_1063_CUST;
/*----------------------------------------------------------------------------*/
typedef struct {
	uint16_t action;
	uint16_t part;
	int32_t data[0];
} KX126_1063_SET_CUST;
/*----------------------------------------------------------------------------*/
typedef struct {
	uint16_t action;
	int32_t data[KX126_1063_AXES_NUM];
} KX126_1063_SET_CALI;
/*----------------------------------------------------------------------------*/
typedef KX126_1063_CUST KX126_1063_RESET_CALI;
/*----------------------------------------------------------------------------*/
typedef union {
	uint32_t data[10];
	KX126_1063_CUST cust;
	KX126_1063_SET_CUST setCust;
	KX126_1063_SET_CALI setCali;
	KX126_1063_RESET_CALI resetCali;
} KX126_1063_CUST_DATA;
/*----------------------------------------------------------------------------*/
//Gionee <GN_BSP_SNS> <tangjh> <20160228> add for IDXXX begin
int KX126_1063_init_pedometer(int enable);
s32 kx126_i2c_write_step(u8 addr, u8 *txbuf, int len );
int kx126_i2c_read_step(u8 addr, u8 *rxbuf, int len);
int KX126_1063_SetPowerMode_step(bool enable);
//Gionee <GN_BSP_SNS> <tangjh> <20160228> add for IDXXX end
#endif
