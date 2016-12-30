//tuwenzan@wind-mobi.com add at 20161128 begin
/*
  * Date: Jan 29th, 2013
  * Revision: V1.0
  *
  *
  * This software program is licensed subject to the GNU General Public License
  * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html
  *
  *
  * (C) Copyright 2013 Bosch Sensortec GmbH 
  * All Rights Reserved
 */

/*
  * File: bma2xx-offset-cali.c
  * 
  * Bosch Sensortec accelerometer offset calibration reference code
  * Apply to BMA222, BMA222E, BMA250, BMA250E, BMA255, BMA280
  *
*/
#include <linux/delay.h>
#include <linux/types.h>
#include "bma2x2.h"

#define BMA2XX_EEPROM_CTRL_REG   0x33
#define BMA2XX_OFFSET_CTRL_REG   0x36
#define BMA2XX_OFFSET_PARAMS_REG 0x37
#define BMA2XX_OFFSET_X			 0x38
#define BMA2XX_OFFSET_Y			 0x39
#define BMA2XX_OFFSET_Z			 0x3A
#define BMA2XX_RANGE_2G			 0x03

extern struct i2c_client *bma2x2_client;

static int bma2xx_fast_compensate(const unsigned char target[3]);
#ifdef BACKUP_OFFSET_OUTSIDE
static int bma2xx_backup_offset(void);
#else
static int bma2xx_eeprom_prog(void);
#endif

/*
  offset fast calibration
  target:
  0 => 0g; 1 => +1g; 2 => -1g; 3 => 0g
*/
int bma2xx_offset_fast_cali(
	unsigned char target_x, 
	unsigned char target_y, 
	unsigned char target_z)
{
	unsigned char target[3];
	int ret = 0;
	/* check arguments */
	if ((target_x > 3) || (target_y > 3) || (target_z > 3))
	{
		return -1;
	}

	/* trigger offset calibration secondly */
	target[0] = target_x;
	target[1] = target_y;
	target[2] = target_z;
	ret = bma2xx_fast_compensate(target);
	printk("tuwenzan bma2xx_fast_compensate ret = %d\n",ret);
	if (ret != 0)
	{
		return -2;
	}

#ifdef BACKUP_OFFSET_OUTSIDE
	/* save offset to NVRAM for backup */
	if (bma2xx_backup_offset() != 0)
	{
		return -3;
	}
#else
	/* save calibration result to EEPROM finally */
	if (bma2xx_eeprom_prog() != 0)
	{
		return -3;
	}
#endif

	
	return 0;
}

/*
  offset fast compensation
  target:
  0 => 0g; 1 => +1g; 2 => -1g; 3 => 0g
*/
static int bma2xx_fast_compensate(
	const unsigned char target[3])
{
	static const int CALI_TIMEOUT = 100;
	int res = 0, timeout = 0;
	unsigned char databuf;
  
	/*** set normal mode, make sure that lowpower_en bit is '0' ***/
	res = BMA2x2_SetPowerMode(bma2x2_client,true);
	if (res != 0)
	{
		return -1;
	}
  
	/*** set +/-2g range ***/
	res = bma2x2_set_range(bma2x2_client,BMA2XX_RANGE_2G);
	if (res <= 0)
	{
		return -2;
	}
  
	/*** set 1000 Hz bandwidth ***/
	res = bma2x2_set_bandwidth(bma2x2_client,BMA2x2_BW_1000HZ);
	if (res <= 0 ) 
	{
		return -3;
	}
	
	/*** set offset target (x/y/z) ***/
	res = bma_i2c_read_block(bma2x2_client,BMA2XX_OFFSET_PARAMS_REG, &databuf,1);
	if (res != 0)
	{
		return -4;
	}
	/* combine three target value */
	databuf &= 0x81; //clean old value
	databuf |= (((target[0] & 0x03) << 1) | ((target[1] & 0x03) << 3) | ((target[2] & 0x03) << 5));
	res = bma_i2c_write_block(bma2x2_client,BMA2XX_OFFSET_PARAMS_REG, &databuf,1);
	if (res != 0)
	{
		return -5;
	}

	/*** trigger x-axis offset compensation ***/
	//printk(KERN_INFO "trigger x-axis offset compensation\n");
	res = bma_i2c_read_block(bma2x2_client,BMA2XX_OFFSET_CTRL_REG, &databuf,1);
	if (res != 0)
	{
		return -6;
	}
	databuf &= 0x9F; //clean old value
	databuf |= 0x01 << 5;
	res = bma_i2c_write_block(bma2x2_client,BMA2XX_OFFSET_CTRL_REG, &databuf,1);
	if (res != 0)
	{
		return -7;
	}

	/*** checking status and waiting x-axis offset compensation done ***/
	timeout = 0;
	do 
	{
		mdelay(2);
		bma_i2c_read_block(bma2x2_client,BMA2XX_OFFSET_CTRL_REG, &databuf,1);
		databuf = (databuf >> 4) & 0x01;  //get cal_rdy bit
		//printk(KERN_INFO "wait 2ms and get cal_rdy = %d\n", databuf);
		if (++timeout == CALI_TIMEOUT)
		{
			//printk(KERN_ERR "check cal_rdy time out\n");
			return -8;
		}
	} while (databuf == 0);

	/*** trigger y-axis offset compensation ***/
	//printk(KERN_INFO "trigger y-axis offset compensation\n");
	res = bma_i2c_read_block(bma2x2_client,BMA2XX_OFFSET_CTRL_REG, &databuf,1);
	if (res != 0)
	{
		return -9;
	}
	databuf &= 0x9F; //clean old value
	databuf |= 0x02 << 5;
	res = bma_i2c_write_block(bma2x2_client,BMA2XX_OFFSET_CTRL_REG, &databuf,1);
	if (res != 0)
	{
		return -10;
	}

	/*** checking status and waiting y-axis offset compensation done ***/
	timeout = 0;
	do 
	{
		mdelay(2);
		bma_i2c_read_block(bma2x2_client,BMA2XX_OFFSET_CTRL_REG, &databuf,1);
		databuf = (databuf >> 4) & 0x01;  //get cal_rdy bit
		//printk(KERN_INFO "wait 2ms and get cal_rdy = %d\n", databuf);
		if (++timeout == CALI_TIMEOUT)
		{
			//printk(KERN_ERR "check cal_rdy time out\n");
			return -11;
		}
	} while (databuf == 0);
  
	/*** trigger z-axis offset compensation ***/
	//printk(KERN_INFO "trigger z-axis offset compensation\n");
	res = bma_i2c_read_block(bma2x2_client,BMA2XX_OFFSET_CTRL_REG, &databuf,1);
	if (res != 0)
	{
		return -12;
	}
	databuf &= 0x9F; //clean old value
	databuf |= 0x03 << 5;
	res = bma_i2c_write_block(bma2x2_client,BMA2XX_OFFSET_CTRL_REG, &databuf,1);
	if (res != 0)
	{
	  return -13;
	}

	/*** checking status and waiting z-axis offset compensation done ***/
	timeout = 0;
	do 
	{
		mdelay(2);
		bma_i2c_read_block(bma2x2_client,BMA2XX_OFFSET_CTRL_REG, &databuf,1);
		databuf = (databuf >> 4) & 0x01;  //get cal_rdy bit
		//printk(KERN_INFO "wait 2ms and get cal_rdy = %d\n", databuf);
		if (++timeout == CALI_TIMEOUT)
		{
			//printk(KERN_ERR "check cal_rdy time out\n");
			return -14;
		}
	} while (databuf == 0);
	printk("tuwenzan exit bma2xx_fast_compensate\n");
	return 0;
}

#ifndef BACKUP_OFFSET_OUTSIDE
static int bma2xx_eeprom_prog(void)
{
	static const int PROG_TIMEOUT = 50;
	int res = 0, timeout = 0;
	unsigned char databuf;    

	/*** unlock EEPROM: write '1' to bit (0x33) nvm_prog_mode ***/
	//printk(KERN_INFO "unlock EEPROM\n");
	res = bma_i2c_read_block(bma2x2_client,BMA2XX_EEPROM_CTRL_REG, &databuf,1);
	if (res != 0)
	{
		return -1;
	}
	databuf |= 0x01;
	res = bma_i2c_write_block(bma2x2_client,BMA2XX_EEPROM_CTRL_REG, &databuf,1);
	if (res != 0)
	{
		return -2;
	}

	/*** need to delay ??? ***/

	/*** trigger the write process: write '1' to bit (0x33) nvm_prog_trig and keep '1' in bit (0x33) nvm_prog_mode ***/
	//printk(KERN_INFO "trigger EEPROM write process\n");
	res = bma_i2c_read_block(bma2x2_client,BMA2XX_EEPROM_CTRL_REG, &databuf,1);
	if (res != 0)
	{
		res = -3;
		goto __lock_eeprom__;
	}
	databuf |= 0x02;
	res = bma_i2c_write_block(bma2x2_client,BMA2XX_EEPROM_CTRL_REG, &databuf,1);
	if (res != 0)
	{
		res = -4;
		goto __lock_eeprom__;
	}

	/*** check the write status by reading bit (0x33) nvm_rdy
	while nvm_rdy = '0', the write process is still enduring; if nvm_rdy = '1', then writing is completed ***/
	do 
	{
		mdelay(2);
		bma_i2c_read_block(bma2x2_client,BMA2XX_EEPROM_CTRL_REG, &databuf,1);
		databuf = (databuf >> 2) & 0x01;  //get nvm_rdy bit
		//printk(KERN_INFO "wait 2ms and get nvm_rdy = %d\n", databuf);
		if (++timeout == PROG_TIMEOUT)
		{
			//printk(KERN_ERR "check nvm_rdy time out\n");
			res = -5;
			goto __lock_eeprom__;
		}
	} while (databuf == 0);

	/*** lock EEPROM: write '0' to nvm_prog_mode ***/
__lock_eeprom__:
	//printk(KERN_INFO "lock EEPROM\n");
	res = bma_i2c_read_block(bma2x2_client,BMA2XX_EEPROM_CTRL_REG, &databuf,1);
	if(res != 0)
	{
		return -6;
	}
	databuf &= 0xFE;
	res = bma_i2c_write_block(bma2x2_client,BMA2XX_EEPROM_CTRL_REG, &databuf,1);
	if(res != 0)
	{
		return -7;
	}

	return res;
}
#endif 

#ifdef BACKUP_OFFSET_OUTSIDE
static int bma2xx_backup_offset(void)
{
	unsigned char offset[3];
	int res = 0;
	
	/* read offset out for backup */
	res = bma_i2c_read_block(bma2x2_client,BMA2XX_OFFSET_X, &(offset[0]),1);
	if (res != 0)
	{
		return -1;
	}
	res = bma_i2c_read_block(bma2x2_client,BMA2XX_OFFSET_Y, &(offset[1]),1);
	if (res != 0)
	{
		return -2;
	}
	res = bma_i2c_read_block(bma2x2_client,BMA2XX_OFFSET_Z, &(offset[2]),1);
	if (res != 0)
	{
		return -3;
	}

	/* save offset into NVRAM */
	
	//TO BE IMPLEMENTED BY CUSTOMER


	return 0;
}

int bma2xx_restore_offset(void)
{
	unsigned char offset[3];
	int res = 0;

	/* load offset from NVRAM */
	
	//TO BE IMPLEMENTED BY CUSTOMER
	

	/* write offset back to sensor */
	res = bma_i2c_write_block(bma2x2_client,BMA2XX_OFFSET_X, &(offset[0]),1);
	if (res != 0)
	{
		return -10;
	}
	res = bma_i2c_write_block(bma2x2_client,BMA2XX_OFFSET_Y, &(offset[1]),1);
	if (res != 0)
	{
		return -11;
	}
	res = bma_i2c_write_block(bma2x2_client,BMA2XX_OFFSET_Z, &(offset[2]),1);
	if (res != 0)
	{
		return -12;
	}

	return 0;
}
#endif
//tuwenzan@wind-mobi.com add at 20161128 end

