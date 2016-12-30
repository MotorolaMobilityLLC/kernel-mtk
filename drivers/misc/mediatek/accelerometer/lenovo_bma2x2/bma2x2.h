//tuwenzan@wind-mobi.com modify at 20161108 begin
/* BMA255 motion sensor driver
 *
 *
 * This software program is licensed subject to the GNU General Public License
 * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html

 * (C) Copyright 2011 Bosch Sensortec GmbH
 * All Rights Reserved
 */

#ifndef BMA2x2_H
#define BMA2x2_H

#include <linux/ioctl.h>
#include <linux/i2c.h> //tuwenzan@wind-mobi.com add at 20161128

/*ChipIDs for BMA2x2 seriers*/
#define BMA222E_CHIPID              0xF8
#define BMA250E_CHIPID              0xF9
#define BMA255_CHIPID               0xFA
#define BMA280_CHIPID               0xFB

/*TYPE for BMA2x2 seriers*/
enum BMA_TYPE {
	BMA222E_TYPE = 0x00,
	BMA250E_TYPE = 0x01,
	BMA255_TYPE = 0x02,
	BMA280_TYPE = 0x03,
	UNKNOWN_TYPE = -1,
	 };



/* 7-bit addr: 0x18 (SDO connected to GND); 0x19 (SDO connected to VDDIO) */
#define BMA2x2_I2C_ADDR				0x18


 /* BMA255 Register Map  (Please refer to BMA255 Specifications) */
#define BMA2x2_REG_DEVID			0x00
#define BMA2x2_REG_OFSX				0x16
#define BMA2x2_REG_OFSX_HIGH		0x1A
#define BMA2x2_REG_BW_RATE			0x10
#define BMA2x2_BW_MASK				0x1f
#define BMA2x2_BW_200HZ				0x0d
#define BMA2x2_BW_100HZ				0x0c
#define BMA2x2_BW_50HZ				0x0b
#define BMA2x2_BW_25HZ				0x0a
#define BMA2x2_REG_POWER_CTL		0x11
#define BMA2x2_REG_DATA_FORMAT		0x0f
#define BMA2x2_RANGE_MASK			0x0f
#define BMA2x2_RANGE_2G				0x03
#define BMA2x2_RANGE_4G				0x05
#define BMA2x2_RANGE_8G				0x08
#define BMA2x2_REG_DATAXLOW			0x02
#define BMA2x2_REG_DATA_RESOLUTION	0x14
#define BMA2x2_MEASURE_MODE			0x80
#define BMA2x2_SELF_TEST			0x32
#define BMA2x2_SELF_TEST_AXIS_X		0x01
#define BMA2x2_SELF_TEST_AXIS_Y		0x02
#define BMA2x2_SELF_TEST_AXIS_Z		0x03
#define BMA2x2_SELF_TEST_POSITIVE	0x00
#define BMA2x2_SELF_TEST_NEGATIVE	0x04
#define BMA2x2_INT_REG_1			0x16
#define BMA2x2_INT_REG_2			0x17

#define BMA2x2_FIFO_MODE_REG                    0x3E
#define BMA2x2_FIFO_DATA_OUTPUT_REG             0x3F
#define BMA2x2_STATUS_FIFO_REG                  0x0E

#define BMA2x2_SUCCESS					0
#define BMA2x2_ERR_I2C					-1
#define BMA2x2_ERR_CLIENT				-2
#define BMA2x2_ERR_STATUS				-3
#define BMA2x2_ERR_SETUP_FAILURE		-4
#define BMA2x2_ERR_GETGSENSORDATA		-5
#define BMA2x2_ERR_IDENTIFICATION		-6
#define BMA2x2_BUFSIZE					256

/* power mode */
#define BMA2x2_MODE_CTRL_REG			0x11

#define BMA2x2_MODE_CTRL__POS			5
#define BMA2x2_MODE_CTRL__LEN			3
#define BMA2x2_MODE_CTRL__MSK			0xE0
#define BMA2x2_MODE_CTRL__REG			BMA2x2_MODE_CTRL_REG

#define BMA2x2_LOW_POWER_CTRL_REG		0x12

#define BMA2x2_LOW_POWER_MODE__POS		6
#define BMA2x2_LOW_POWER_MODE__LEN		1
#define BMA2x2_LOW_POWER_MODE__MSK		0x40
#define BMA2x2_LOW_POWER_MODE__REG		BMA2x2_LOW_POWER_CTRL_REG

/* range */
#define BMA2x2_RANGE_SEL_REG			0x0F

#define BMA2x2_RANGE_SEL__POS			0
#define BMA2x2_RANGE_SEL__LEN			4
#define BMA2x2_RANGE_SEL__MSK			0x0F
#define BMA2x2_RANGE_SEL__REG			BMA2x2_RANGE_SEL_REG

/* bandwidth */
#define BMA2x2_BW_7_81HZ			0x08
#define BMA2x2_BW_15_63HZ			0x09
#define BMA2x2_BW_31_25HZ			0x0A
#define BMA2x2_BW_62_50HZ			0x0B
#define BMA2x2_BW_125HZ				0x0C
#define BMA2x2_BW_250HZ				0x0D
#define BMA2x2_BW_500HZ				0x0E
#define BMA2x2_BW_1000HZ			0x0F

#define BMA2x2_BW_SEL_REG			0x10

#define BMA2x2_BANDWIDTH__POS			0
#define BMA2x2_BANDWIDTH__LEN			5
#define BMA2x2_BANDWIDTH__MSK			0x1F
#define BMA2x2_BANDWIDTH__REG			BMA2x2_BW_SEL_REG

#define MAX_REGISTER_MAP 0x3F
#define BMA2x2_SOFTRESET 0xB6
#define BMA2x2_SOFTRESET_REG 0x14
#define CONVERT_M			1
#define CONVERT_M_DIV		4
#define CONVERT_O			1
#define CONVERT_O_DIV		71
#define CONVERT_G			1
#define CONVERT_G_DIV		938
#define CONVERT_VRV			1
#define CONVERT_VRV_DIV	(0x40000000)
#define CONVERT_VLA_DIV	16384
#define CONVERT_VG_DIV 16384

//tuwenzan@wind-mobi.com add at 20161128 begin
//#define BACKUP_OFFSET_OUTSIDE
#define BMA2XX_OFFSET_CALI
extern int BMA2x2_SetPowerMode(struct i2c_client *client, bool enable);
extern int bma2x2_set_range(struct i2c_client *client, unsigned char range);
extern int bma2x2_set_bandwidth(struct i2c_client *client,unsigned char bandwidth);
extern int bma_i2c_read_block(struct i2c_client *client,u8 addr, u8 *data, u8 len);
extern int bma_i2c_write_block(struct i2c_client *client, u8 addr,u8 *data, u8 len);
extern int bma2xx_offset_fast_cali(unsigned char target_x, unsigned char target_y, unsigned char target_z);

#ifdef BACKUP_OFFSET_OUTSIDE
extern int bma2xx_restore_offset(void);
#endif 

//tuwenzan@wind-mobi.com add at 20161128 end
#endif
//tuwenzan@wind-mobi.com modify at 20161108 end